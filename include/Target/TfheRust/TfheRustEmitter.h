#ifndef INCLUDE_TARGET_TFHERUST_TFHERUSTEMITTER_H_
#define INCLUDE_TARGET_TFHERUST_TFHERUSTEMITTER_H_

#include <string>
#include <string_view>

#include "include/Analysis/SelectVariableNames/SelectVariableNames.h"
#include "include/Dialect/TfheRust/IR/TfheRustDialect.h"
#include "include/Dialect/TfheRust/IR/TfheRustOps.h"
#include "llvm/include/llvm/Support/raw_ostream.h"       // from @llvm-project
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"    // from @llvm-project
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"   // from @llvm-project
#include "mlir/include/mlir/Dialect/MemRef/IR/MemRef.h"  // from @llvm-project
#include "mlir/include/mlir/Dialect/Tensor/IR/Tensor.h"  // from @llvm-project
#include "mlir/include/mlir/IR/BuiltinOps.h"             // from @llvm-project
#include "mlir/include/mlir/IR/Operation.h"              // from @llvm-project
#include "mlir/include/mlir/IR/Types.h"                  // from @llvm-project
#include "mlir/include/mlir/IR/Value.h"                  // from @llvm-project
#include "mlir/include/mlir/IR/ValueRange.h"             // from @llvm-project
#include "mlir/include/mlir/Support/IndentedOstream.h"   // from @llvm-project
#include "mlir/include/mlir/Support/LLVM.h"              // from @llvm-project
#include "mlir/include/mlir/Support/LogicalResult.h"     // from @llvm-project

namespace mlir {
namespace heir {
namespace tfhe_rust {

void registerToTfheRustTranslation();

/// Translates the given operation to TfheRust.
::mlir::LogicalResult translateToTfheRust(::mlir::Operation *op,
                                          llvm::raw_ostream &os);

class TfheRustEmitter {
 public:
  TfheRustEmitter(raw_ostream &os, SelectVariableNames *variableNames);

  LogicalResult translate(::mlir::Operation &operation);

 private:
  /// Output stream to emit to.
  raw_indented_ostream os;

  /// Pre-populated analysis selecting unique variable names for all the SSA
  /// values.
  SelectVariableNames *variableNames;

  // Server key arg to create default values when initializing arrays
  std::string serverKeyArg_;

  // Functions for printing individual ops
  LogicalResult printOperation(::mlir::ModuleOp op);
  LogicalResult printOperation(::mlir::arith::ConstantOp op);
  LogicalResult printOperation(::mlir::func::FuncOp op);
  LogicalResult printOperation(::mlir::func::ReturnOp op);
  LogicalResult printOperation(AddOp op);
  LogicalResult printOperation(BitAndOp op);
  LogicalResult printOperation(CreateTrivialOp op);
  LogicalResult printOperation(tensor::ExtractOp op);
  LogicalResult printOperation(tensor::FromElementsOp op);
  LogicalResult printOperation(memref::AllocOp op);
  LogicalResult printOperation(memref::LoadOp op);
  LogicalResult printOperation(memref::StoreOp op);
  LogicalResult printOperation(ApplyLookupTableOp op);
  LogicalResult printOperation(GenerateLookupTableOp op);
  LogicalResult printOperation(ScalarLeftShiftOp op);

  // Helpers for above
  LogicalResult printSksMethod(::mlir::Value result, ::mlir::Value sks,
                               ::mlir::ValueRange nonSksOperands,
                               std::string_view op,
                               SmallVector<std::string> operandTypes = {});

  // Emit a TfheRust type
  LogicalResult emitType(Type type);
  FailureOr<std::string> convertType(Type type);
  // Emit a default value for the given type
  FailureOr<std::string> defaultValue(Type type);

  void emitAssignPrefix(Value result, bool mut = false, std::string type = "");
};

}  // namespace tfhe_rust
}  // namespace heir
}  // namespace mlir

#endif  // INCLUDE_TARGET_TFHERUST_TFHERUSTEMITTER_H_
