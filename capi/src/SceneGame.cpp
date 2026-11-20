// Project factory linked into the generic native player. It runs a saved scene-only project
// using the shared engine. The C API creates its scene-only fallback independently.

#include "pixie/runtime/SceneProject.hpp"

#include <memory>

std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return std::make_unique<pixie::SceneProject>();
}
