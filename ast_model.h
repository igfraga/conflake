
#include <map>
#include <ast_entry.h>

namespace ast {

struct Model {
    std::map<EntryId, Entry> mEntries;
};

}  // namespace ast
