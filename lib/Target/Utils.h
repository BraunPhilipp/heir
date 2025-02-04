#ifndef HEIR_INCLUDE_TARGET_UTIL_H_
#define HEIR_INCLUDE_TARGET_UTIL_H_

#include "mlir/include/mlir/IR/TypeRange.h"           // from @llvm-project
#include "mlir/include/mlir/IR/Types.h"               // from @llvm-project
#include "mlir/include/mlir/IR/Value.h"               // from @llvm-project
#include "mlir/include/mlir/IR/ValueRange.h"          // from @llvm-project
#include "mlir/include/mlir/Support/LogicalResult.h"  // from @llvm-project

namespace mlir {
namespace heir {

// Return a comma-separated string containing the values in the given
// ValueRange, with each value being converted to a string by the given mapping
// function.
std::string commaSeparatedValues(
    ValueRange values, std::function<std::string(Value)> valueToString);

// Return a comma-separated string containing the types in a given TypeRange,
// or failure if the mapper fails to convert any of the types.
FailureOr<std::string> commaSeparatedTypes(
    TypeRange types, std::function<FailureOr<std::string>(Type)> typeToString);

}  // namespace heir
}  // namespace mlir

#endif  // HEIR_INCLUDE_TARGET_UTIL_H_
