
#pragma once

#include <pom_ops.h>
#include <pom_type.h>
#include <tl/expected.hpp>
#include "llvm/IR/IRBuilder.h"

namespace llvm {
class Value;
}

namespace pol {

namespace basicoperators {

struct Err {
    std::string m_desc;
};

using ValueGenerator = std::function<tl::expected<llvm::Value*, Err>(llvm::IRBuilderBase*)>;

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase*               builder,
                                           const pom::ops::OpInfo&            op_info,
                                           const std::vector<ValueGenerator>& operands);

tl::expected<std::vector<llvm::Value*>, Err> execute(const std::vector<ValueGenerator>& operands,
                                                     llvm::IRBuilderBase*               builder);

}  // namespace basicoperators

}  // namespace pol
