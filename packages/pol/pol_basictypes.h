#pragma once

#include <pom_type.h>
#include <tl/expected.hpp>

namespace llvm {
class LLVMContext;
class Type;
}  // namespace llvm

namespace pol {

namespace basictypes {

struct Err
{
    std::string m_desc;
};

tl::expected<llvm::Type*, Err> getType(llvm::LLVMContext* context, const pom::Type& type);

}  // namespace basictypes

}  // namespace pol
