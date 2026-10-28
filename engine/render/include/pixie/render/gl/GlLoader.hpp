#pragma once

#include "pixie/render/Export.hpp"

namespace pixie {

// Loads the OpenGL 4.5 core functions (through GLEW) for the context current on this thread, logs
// the GL version, renderer, and vendor, and routes GL debug messages to the log when the context is
// a debug context. pixie::Run calls it for its window; a host that creates its own context calls it
// once, with that context current, before anything else in the engine touches GL. Returns false,
// after logging why, if the functions cannot be loaded.
PIXIE_RENDER_API bool LoadGl();

} // namespace pixie
