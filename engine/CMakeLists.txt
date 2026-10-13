# Provisional engine library: window, GL context, and main loop.
# Its layout changes when the module split (PIXIE-PROPOSAL-001) is decided.

add_library(pixie_engine STATIC
    src/FrameLimiter.cpp
    src/GlDebugOutput.cpp
    src/LibraryCheck.cpp
    src/Log.cpp
    src/RateMeter.cpp
    src/Run.cpp
    src/Window.cpp
)
add_library(Pixie::Engine ALIAS pixie_engine)
set_target_properties(pixie_engine PROPERTIES FOLDER engine)

target_include_directories(pixie_engine PUBLIC include)
target_link_libraries(pixie_engine PUBLIC glew pixie::pixie pixie::math EnTT::EnTT PRIVATE glfw)
target_compile_definitions(pixie_engine PRIVATE GLFW_INCLUDE_NONE)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(pixie_engine)
