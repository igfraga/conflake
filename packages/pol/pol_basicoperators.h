
#pragma once

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

tl::expected<llvm::Value*, Err> buildBinOp(llvm::IRBuilderBase* builder, char op,
                                           const pom::Type& type, llvm::Value* lv, llvm::Value* rv);

}  // namespace basicoperators

}  // namespace pol
