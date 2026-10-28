#pragma once

#include "pixie/render/gl/Gl.hpp"

#include <string_view>

// Compiles and links a vertex and fragment shader. Returns 0 and logs the error on failure.
GLuint CompileProgram(std::string_view vertexSource, std::string_view fragmentSource);
