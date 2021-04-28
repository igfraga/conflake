
#pragma once

#include <map>
#include <pom_entry.h>

namespace pom {

struct Model {
    std::map<EntryId, Entry> mEntries;
};

}  // namespace ast
