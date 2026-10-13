#pragma once

#include "pixie/Gl.hpp"

#include <string_view>

namespace pixie {

// A linked vertex and fragment shader program. Owns the GL object: Destroy it while the context is
// still current (in App::Stop), since a destructor runs too late for that.
class ShaderProgram {
public:
    ShaderProgram() = default;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;
    ~ShaderProgram();

    // Compiles and links; on failure logs the compiler's message and returns false. `name` only
    // labels the log.
    bool Create(std::string_view name, std::string_view vertexSource,
                std::string_view fragmentSource);
    void Destroy();

    GLuint Id() const { return id_; }
    bool IsValid() const { return id_ != 0; }

private:
    GLuint id_ = 0;
};

} // namespace pixie
