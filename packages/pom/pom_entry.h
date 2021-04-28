
#pragma once

#include <string>
#include <vector>

namespace pom {

using EntryId = int64_t;

struct Entry {
    EntryId              mId;
    std::string          mFuncName;
    std::vector<EntryId> mInputs;
};

}  // namespace pom
