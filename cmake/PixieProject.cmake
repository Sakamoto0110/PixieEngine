# Included by the CMakeLists.txt of a project with code, after project(): adds the engine found at
# PIXIE_ENGINE_DIR, without its samples or tests, and puts every binary the project builds in
# <project>/bin/<config>/. The editor passes PIXIE_ENGINE_DIR when it builds the project, with the
# engine it runs from, so the project never records where the engine is.

if(NOT IS_DIRECTORY "${PIXIE_ENGINE_DIR}" OR NOT EXISTS "${PIXIE_ENGINE_DIR}/cmake/PixieProject.cmake")
    message(FATAL_ERROR
        "PIXIE_ENGINE_DIR must name PixieEngine's folder (it is \"${PIXIE_ENGINE_DIR}\"). The editor "
        "passes it when it builds the project; by hand: cmake -DPIXIE_ENGINE_DIR=<engine> ...")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_subdirectory(${PIXIE_ENGINE_DIR} pixie-engine)
