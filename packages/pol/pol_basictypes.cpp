
#include <pol_basictypes.h>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

#include <fmt/format.h>

namespace pol {

namespace basictypes {

tl::expected<llvm::Type*, Err> getType(llvm::LLVMContext* context, const pom::Type& type)
{
    if (type.mangled() == "real") {
        return llvm::Type::getDoubleTy(*context);
    } else if (type.mangled() == "integer") {
        return llvm::Type::getInt64Ty(*context);
    } else if (type.mangled() == "boolean") {
        return llvm::Type::getInt1Ty(*context);
    } else if (type.mangled() == "__list_real") {
        return llvm::PointerType::get(llvm::Type::getDoubleTy(*context), 0);
    } else if (type.mangled() == "__list_integer") {
        return llvm::PointerType::get(llvm::Type::getInt64Ty(*context), 0);
    } else if (type.mangled() == "integer") {
        return llvm::Type::getInt64Ty(*context);
    }

    return tl::make_unexpected(Err{fmt::format("type not supported: {0}", type.description())});
}

}  // namespace basictypes

}  // namespace pol
