// The game of the engine's generic C API library, pixie-scene-capi: a project whose World is only
// its scene, which is how a project folder without code runs.

#include "pixie/runtime/SceneProject.hpp"

#include <memory>

std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return std::make_unique<pixie::SceneProject>();
}
