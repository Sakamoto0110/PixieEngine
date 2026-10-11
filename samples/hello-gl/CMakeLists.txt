add_executable(hello-gl
    src/Ball.cpp
    src/BallRenderer.cpp
    src/HelloGlApp.cpp
    src/Shader.cpp
    src/TextRenderer.cpp
    src/main.cpp
)

set_target_properties(hello-gl PROPERTIES FOLDER samples)

target_link_libraries(hello-gl PRIVATE Pixie::Engine stb_easy_font)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(hello-gl)
