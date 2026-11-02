# Included by the CMakeLists.txt of a project with code, after project(): adds the engine found at
# PIXIE_ENGINE_DIR, without its samples or tests, and puts every binary the project builds in
# <project>/bin/<config>/. The editor passes PIXIE_ENGINE_DIR when it builds the project, with the
# engine it runs from, so the project never records where the engine is.

if(NOT IS_DIRECTORY "${PIXIE_ENGINE_DIR}" OR NOT EXISTS "${PIXIE_ENGINE_DIR}/cmake/PixieProject.cmake")
    message(FATAL_ERROR
        "PIXIE_ENGINE_DIR must name PixieEngine's folder (it is \"${PIXIE_ENGINE_DIR}\"). The editor "
        "passes it when it builds the project; by hand: cmake -DPIXIE_ENGINE_DIR=<engine> ...")
endif()

set(PIXIE_OUTPUT_ROOT "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "Root for native runtime and shared-library output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PIXIE_OUTPUT_ROOT}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PIXIE_OUTPUT_ROOT}/$<CONFIG>)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
add_subdirectory(${PIXIE_ENGINE_DIR} pixie-engine)

# The editor's managed component registry attaches to the project's existing game library.
# Never guess an executable or attach generated code to an engine target.
function(pixie_project_components)
    get_property(targets DIRECTORY PROPERTY PIXIE_GAME_CODE_TARGETS)
    list(LENGTH targets count)
    if(NOT count EQUAL 1)
        message(FATAL_ERROR "Generated components require exactly one pixie_game in this directory. Wire the registry explicitly for a custom multi-game project.")
    endif()
    list(GET targets 0 PIXIE_COMPONENT_TARGET)
    set(registry "${CMAKE_CURRENT_SOURCE_DIR}/.pixie-code/Components.cmake")
    if(NOT EXISTS "${registry}")
        message(FATAL_ERROR "Component source registry is missing: ${registry}. Restore the generated project-code files.")
    endif()
    include("${registry}")
endfunction()
