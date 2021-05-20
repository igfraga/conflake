
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

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase*    builder,
                                           const pom::ops::OpInfo& op_info, llvm::Value* lv,
                                           llvm::Value* rv);

}  // namespace basicoperators

}  // namespace pol
