add_library(pixie_app STATIC
    src/DebugConsole.cpp
    src/DebugOverlay.cpp
    src/Run.cpp
    src/RunOptions.cpp
)
add_library(Pixie::App ALIAS pixie_app)
set_target_properties(pixie_app PROPERTIES FOLDER engine)
target_include_directories(pixie_app PUBLIC include)
target_link_libraries(pixie_app PUBLIC Pixie::Core Pixie::Platform Pixie::Render)
# Console text tessellation uses private stb headers.
target_include_directories(pixie_app SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external/stb)
target_compile_definitions(pixie_app PRIVATE GLFW_INCLUDE_NONE)
pixie_target_defaults(pixie_app)
