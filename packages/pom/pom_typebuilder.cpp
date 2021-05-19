
#include <pom_typebuilder.h>

#include <pom_basictypes.h>
#include <pom_listtype.h>

namespace pom {

namespace types {

TypeCSP build(const ast::TypeDesc& type) {
    if (type.m_template_args.empty()) {
        if (type.m_name == "real") {
            return real();
        } else if (type.m_name == "integer") {
            return integer();
        } else if (type.m_name == "boolean") {
            return boolean();
        }
        return nullptr;
    }

    if (type.m_name == "list") {
        if (type.m_template_args.size() != 1ull) {
            return nullptr;
        }
        auto t = build(*type.m_template_args[0]);
        if (!t) {
            return nullptr;
        }
        return list(t);
    }
    return nullptr;
}

}  // namespace types

}  // namespace pom
