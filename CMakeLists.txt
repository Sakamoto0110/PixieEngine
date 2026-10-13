cmake_minimum_required(VERSION 3.25)

project(PixieEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Samples only exercise what the engine already has; turn them off to build the engine alone.
option(PIXIE_BUILD_SAMPLES "Build the samples under samples/" ON)
# Not PIXIE_BUILD_TESTS: PixieLib owns that name for its own tests.
option(PIXIE_ENGINE_BUILD_TESTS "Build the tests under tests/ and register them with CTest" ON)

# Every executable (and, later, shared library) lands in bin/<config>/ at the repository root,
# away from the generated build tree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/$<CONFIG>)

# MSVC: link the C++ runtime statically (/MT, /MTd) into every target, third-party ones included, so
# an executable runs without the Visual C++ Redistributable installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Visual Studio: group projects into solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(runtime)
add_subdirectory(capi)
if(PIXIE_ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(PIXIE_BUILD_SAMPLES)
    add_subdirectory(samples)
    # F5 starts the sample.
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT hello-gl)
endif()

# PixieEngine.slnx at the repository root; see the module for what it shows and why.
include(cmake/PixieRootSolution.cmake)
pixie_write_root_solution()
