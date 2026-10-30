add_library(pixie_app OBJECT
    src/DebugConsole.cpp
    src/DebugOverlay.cpp
    src/Run.cpp
    src/RunOptions.cpp
)
add_library(pixie_app_api INTERFACE)
add_library(Pixie::App ALIAS pixie_app_api)
target_link_libraries(pixie_app_api INTERFACE pixie_shared)
set_target_properties(pixie_app PROPERTIES FOLDER engine)
target_include_directories(pixie_app PUBLIC include)
target_link_libraries(pixie_app PUBLIC pixie_core pixie_platform pixie_render)
# Console text tessellation uses private stb headers.
target_include_directories(pixie_app SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external/stb)
target_compile_definitions(pixie_app PRIVATE GLFW_INCLUDE_NONE)
pixie_target_defaults(pixie_app)
