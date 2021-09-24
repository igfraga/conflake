
#include <pol_basictypes.h>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

#include <fmt/format.h>

namespace pol {

namespace basictypes {

inline bool starts_with(std::string_view s, std::string_view prefix)
{
    return std::equal(prefix.begin(), prefix.end(), s.begin());
}

inline tl::expected<std::vector<llvm::Type*>, Err> convertTemplateArgs(llvm::LLVMContext* context,
                                                                       const pom::Type&   type)
{
    auto                     templ_args = type.templateArgs();
    std::vector<llvm::Type*> llvm_templ_args;
    for (auto& ty : templ_args) {
        auto llvm_ty = getType(context, *ty);
        if (!llvm_ty) {
            return tl::make_unexpected(llvm_ty.error());
        }
        llvm_templ_args.push_back(std::move(*llvm_ty));
    }
    return llvm_templ_args;
}

tl::expected<llvm::Type*, Err> getType(llvm::LLVMContext* context, const pom::Type& type)
{
    if (type.mangled() == "real") {
        return llvm::Type::getDoubleTy(*context);
    } else if (type.mangled() == "integer") {
        return llvm::Type::getInt64Ty(*context);
    } else if (type.mangled() == "boolean") {
        return llvm::Type::getInt1Ty(*context);
    } else if (starts_with(type.mangled(), "__list_")) {
        auto llvm_templ_args = convertTemplateArgs(context, type);
        if (!llvm_templ_args) {
            return tl::make_unexpected(llvm_templ_args.error());
        }
        return llvm::PointerType::get((*llvm_templ_args)[0], 0);
    } else if (starts_with(type.mangled(), "__function__")) {
        auto function_type = getFunctionType(context, type);
        if (!function_type) {
            return tl::make_unexpected(function_type.error());
        }
        return llvm::PointerType::get(*function_type, 0);
    }

    return tl::make_unexpected(Err{fmt::format("type not supported: {0}", type.description())});
}

tl::expected<llvm::FunctionType*, Err> getFunctionType(llvm::LLVMContext* context,
                                                       const pom::Type&   type)
{
    auto llvm_templ_args = convertTemplateArgs(context, type);
    if (!llvm_templ_args) {
        return tl::make_unexpected(llvm_templ_args.error());
    }
    auto llvm_ret_type = (*llvm_templ_args)[0];
    llvm_templ_args->erase(llvm_templ_args->begin());
    return llvm::FunctionType::get(llvm_ret_type, *llvm_templ_args, false);
}

}  // namespace basictypes

}  // namespace pol
