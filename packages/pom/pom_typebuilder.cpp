
#include <pom_typebuilder.h>

#include <pom_basictypes.h>
#include <pom_functiontype.h>
#include <pom_listtype.h>

#include <fmt/format.h>
#include <map>

namespace pom {

namespace types {

tl::expected<TypeCSP, TypeError> build(const ast::TypeDesc& type)
{
    struct NArgs
    {
        size_t m_min;
    };

    static const std::map<std::string, std::variant<size_t, NArgs>> types_info{
        {"real", 0ull}, {"integer", 0ull}, {"boolean", 0ull}, {"list", 1ull}, {"fun", NArgs{1}}};

    auto info = types_info.find(type.m_name);
    if (info == types_info.end()) {
        return tl::make_unexpected(TypeError{fmt::format("Unknown type: {0}", type.m_name)});
    }

    if (std::holds_alternative<size_t>(info->second)) {
        if (std::get<size_t>(info->second) != type.m_template_args.size()) {
            return tl::make_unexpected(
                TypeError{fmt::format("Bad number of template args ({0}) for type: {1}",
                                      type.m_template_args.size(), type.m_name)});
        }
    } else if (std::holds_alternative<NArgs>(info->second)) {
        if (std::get<NArgs>(info->second).m_min > type.m_template_args.size()) {
            return tl::make_unexpected(
                TypeError{fmt::format("Bad number of template args ({0}) for type: {1}",
                                      type.m_template_args.size(), type.m_name)});
        }
    }

    std::vector<TypeCSP> template_types;
    for (auto& targ : type.m_template_args) {
        auto btarg = build(*targ);
        if (!btarg) {
            return btarg;
        }
        template_types.push_back(std::move(*btarg));
    }

    if (type.m_name == "real") {
        return real();
    } else if (type.m_name == "integer") {
        return integer();
    } else if (type.m_name == "boolean") {
        return boolean();
    } else if (type.m_name == "list") {
        return list(template_types[0]);
    } else if (type.m_name == "fun") {
        Function function_type;
        function_type.m_ret_type = template_types[0];
        for (size_t i = 1ull; i < template_types.size(); i++) {
            function_type.m_arg_types.push_back(template_types[i]);
        }
        return std::make_shared<Function>(std::move(function_type));
    }

    return tl::make_unexpected(
        TypeError{fmt::format("Unknown type (should have been caught above): {0}", type.m_name)});
    ;
}

}  // namespace types

}  // namespace pom
