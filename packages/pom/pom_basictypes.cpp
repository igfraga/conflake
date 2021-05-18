
#include <pom_basictypes.h>

#include <pom_listtype.h>

namespace pom {

namespace types {

TypeCSP basicTypeFromStr(std::string_view s, const std::optional<std::string>& template_arg) {
    if (template_arg) {
        if (s == "list") {
            if (!template_arg) {
                return nullptr;
            }
            auto t = basicTypeFromStr(*template_arg, std::nullopt);
            if (!t) {
                return nullptr;
            }
            return list(t);
        }
    } else {
        if (s == "real") {
            return real();
        } else if (s == "integer") {
            return integer();
        } else if (s == "boolean") {
            return boolean();
        }
    }
    return nullptr;
}

}  // namespace types

}  // namespace pom
