#include "include/Dialect/CGGI/Transforms/StraightLineVectorizer.h"

#include "include/Dialect/CGGI/IR/CGGIAttributes.h"
#include "include/Dialect/CGGI/IR/CGGIOps.h"
#include "include/Dialect/LWE/IR/LWEAttributes.h"
#include "include/Graph/Graph.h"
#include "llvm/include/llvm/ADT/TypeSwitch.h"            // from @llvm-project
#include "llvm/include/llvm/Support/Debug.h"             // from @llvm-project
#include "mlir/include/mlir/Analysis/SliceAnalysis.h"    // from @llvm-project
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"    // from @llvm-project
#include "mlir/include/mlir/Dialect/Tensor/IR/Tensor.h"  // from @llvm-project
#include "mlir/include/mlir/IR/Visitors.h"               // from @llvm-project
#include "mlir/include/mlir/Support/LLVM.h"              // from @llvm-project
#include "mlir/include/mlir/Transforms/TopologicalSortUtils.h"  // from @llvm-project

#define DEBUG_TYPE "straight-line-vectorizer"

namespace mlir {
namespace heir {
namespace cggi {

#define GEN_PASS_DEF_STRAIGHTLINEVECTORIZER
#include "include/Dialect/CGGI/Transforms/Passes.h.inc"

/// Returns true if the two operations can be combined into a single vectorized
/// operation.
bool areCompatible(Operation *lhs, Operation *rhs) {
  if (lhs->getName() != rhs->getName() ||
      lhs->getDialect() != rhs->getDialect() ||
      lhs->getResultTypes() != rhs->getResultTypes()) {
    return false;
  }

  return llvm::TypeSwitch<Operation *, bool>(lhs)
      .Case<AndOp, OrOp, XorOp>([&](auto op) { return true; })
      .Case<Lut2Op>([&](auto op) {
        return cast<Lut2Op>(rhs).getLookupTable() == op.getLookupTable();
      })
      .Case<Lut3Op>([&](auto op) {
        return cast<Lut3Op>(rhs).getLookupTable() == op.getLookupTable();
      })
      .Default([&](Operation *) {
        lhs->emitOpError("Unsupported check for vectorizable compatibility.");
        return false;
      });
}

bool tryVectorizeBlock(Block *block) {
  graph::Graph<Operation *> graph;
  for (auto &op : block->getOperations()) {
    if (!isa<CGGIDialect>(op.getDialect()) ||
        !op.hasTrait<OpTrait::Elementwise>()) {
      continue;
    }

    graph.addVertex(&op);
    SetVector<Operation *> backwardSlice;
    BackwardSliceOptions options;
    options.omitBlockArguments = true;
    getBackwardSlice(&op, &backwardSlice, options);
    for (auto *upstreamDep : backwardSlice) {
      // An edge from upstreamDep to `op` means that upstreamDep must be
      // computed before `op`.
      graph.addEdge(upstreamDep, &op);
    }
  }

  if (graph.empty()) {
    return false;
  }

  auto result = graph.sortGraphByLevels();
  assert(succeeded(result) &&
         "Only possible failure is a cycle in the SSA graph!");
  auto levels = result.value();

  LLVM_DEBUG({
    llvm::dbgs()
        << "Found operations to vectorize. In topo-sorted level order:\n";
    int level_num = 0;
    for (const auto &level : levels) {
      llvm::dbgs() << "\nLevel " << level_num++ << ":\n";
      for (auto op : level) {
        llvm::dbgs() << " - " << *op << "\n";
      }
    }
  });

  bool madeReplacement = false;
  for (const auto &level : levels) {
    DenseMap<Operation *, SmallVector<Operation *, 4>> compatibleOps;
    for (auto *op : level) {
      bool foundCompatible = false;
      for (auto &[key, bucket] : compatibleOps) {
        if (areCompatible(key, op)) {
          compatibleOps[key].push_back(op);
          foundCompatible = true;
        }
      }
      if (!foundCompatible) {
        compatibleOps[op].push_back(op);
      }
    }
    LLVM_DEBUG(llvm::dbgs()
               << "Partitioned level of size " << level.size() << " into "
               << compatibleOps.size() << " groups of compatible ops\n");

    for (auto &[key, bucket] : compatibleOps) {
      if (bucket.size() < 2) {
        continue;
      }

      LLVM_DEBUG({
        llvm::dbgs() << "Vectorizing ops:\n";
        for (auto op : bucket) {
          llvm::dbgs() << " - " << *op << "\n";
        }
      });

      OpBuilder builder(bucket.back());
      // relies on CGGI ops having a single result type
      Type elementType = key->getResultTypes()[0];
      RankedTensorType tensorType = RankedTensorType::get(
          {static_cast<int64_t>(bucket.size())}, elementType);

      SmallVector<Value, 4> vectorizedOperands;
      for (int operandIndex = 0; operandIndex < key->getNumOperands();
           ++operandIndex) {
        SmallVector<Value, 4> operands;
        operands.reserve(bucket.size());
        for (auto *op : bucket) {
          operands.push_back(op->getOperand(operandIndex));
        }
        auto fromElementsOp = builder.create<tensor::FromElementsOp>(
            key->getLoc(), tensorType, operands);
        vectorizedOperands.push_back(fromElementsOp.getResult());
      }

      Operation *vectorizedOp = builder.clone(*key);
      vectorizedOp->setOperands(vectorizedOperands);
      vectorizedOp->getResult(0).setType(tensorType);

      int bucketIndex = 0;
      for (auto *op : bucket) {
        auto extractionIndex = builder.create<arith::ConstantOp>(
            op->getLoc(), builder.getIndexAttr(bucketIndex));
        auto extractOp = builder.create<tensor::ExtractOp>(
            op->getLoc(), elementType, vectorizedOp->getResult(0),
            extractionIndex.getResult());
        op->replaceAllUsesWith(ValueRange{extractOp.getResult()});
        bucketIndex++;
      }

      for (auto *op : bucket) {
        op->erase();
      }
      madeReplacement = true;
    }
  }

  return madeReplacement;
}

struct StraightLineVectorizer
    : impl::StraightLineVectorizerBase<StraightLineVectorizer> {
  using StraightLineVectorizerBase::StraightLineVectorizerBase;

  void runOnOperation() override {
    getOperation()->walk<WalkOrder::PreOrder>([&](Block *block) {
      if (tryVectorizeBlock(block)) {
        sortTopologically(block);
      }
    });
  }
};

}  // namespace cggi
}  // namespace heir
}  // namespace mlir
