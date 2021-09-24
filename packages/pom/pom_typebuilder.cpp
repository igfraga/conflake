
#include <pom_typebuilder.h>

#include <pom_basictypes.h>
#include <pom_functiontype.h>
#include <pom_listtype.h>

#include <map>

namespace pom {

namespace types {

TypeCSP build(const ast::TypeDesc& type)
{
    struct NArgs
    {
        size_t m_min;
    };

    static const std::map<std::string, std::variant<size_t, NArgs>> types_info{
        {"real", 0ull}, {"integer", 0ull}, {"boolean", 0ull}, {"list", 1ull}, {"fun", NArgs{1}}};

    auto info = types_info.find(type.m_name);
    if (info == types_info.end()) {
        return nullptr;
    }

    if (std::holds_alternative<size_t>(info->second)) {
        if (std::get<size_t>(info->second) != type.m_template_args.size()) {
            return nullptr;
        }
    } else if (std::holds_alternative<NArgs>(info->second)) {
        if (std::get<NArgs>(info->second).m_min > type.m_template_args.size()) {
            return nullptr;
        }
    }

    std::vector<TypeCSP> template_types;
    for (auto& targ : type.m_template_args) {
        auto btarg = build(*targ);
        if (!btarg) {
            return nullptr;
        }
        template_types.push_back(std::move(btarg));
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

    return nullptr;
}

}  // namespace types

}  // namespace pom
