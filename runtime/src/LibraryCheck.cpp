// Compiles the headers of the libraries the engine builds on under the engine's own warnings, and
// pins what the engine assumes about them. Nothing here runs.

#include <entt/entity/registry.hpp>
#include <pixie/math/pxMat.hpp>
#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxRect.hpp>

namespace pixie {

namespace {

// The GPU takes float vectors and matrices as they are laid out in memory.
static_assert(sizeof(pxVec2f) == 2 * sizeof(float));
static_assert(sizeof(pxMat4f) == 16 * sizeof(float));

// An entity is a 32-bit value: index and version.
static_assert(sizeof(entt::entity) == 4);

} // namespace

} // namespace pixie
