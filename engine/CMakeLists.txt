# One-way native library boundaries; existing consumers keep the umbrella target.
include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(render)
add_subdirectory(app)
add_library(pixie_engine INTERFACE)
add_library(Pixie::Engine ALIAS pixie_engine)
target_link_libraries(pixie_engine INTERFACE Pixie::App)
