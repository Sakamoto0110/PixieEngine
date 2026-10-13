#pragma once

#include <string>

namespace pixie {

// A label for an entity, for logs and tools. Optional; World::Create adds it when given a name.
struct Name {
    std::string value;
};

} // namespace pixie
