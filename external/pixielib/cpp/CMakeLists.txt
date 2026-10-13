cmake_minimum_required(VERSION 3.20)
project(PixieLib LANGUAGES CXX)

# Only headers: the primitives are templates and small structs, compiled inside whoever includes them.
add_library(pixie INTERFACE)
add_library(pixie::pixie ALIAS pixie)
target_include_directories(pixie INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(pixie INTERFACE cxx_std_20)

# The events, a module of their own (1.10): the engine links pixie::events, and whoever only wants
# the primitives does not pull them. Also only headers, since everything in them is a template.
add_library(pixie_events INTERFACE)
add_library(pixie::events ALIAS pixie_events)
target_include_directories(pixie_events INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/events/include)
target_compile_features(pixie_events INTERFACE cxx_std_20)

# The math, a module of its own (6.1): the vectors and matrices are GLM's, under the names of
# PixieLib, and the engine links pixie::math. GLM is only headers. It comes pinned to 1.0.3, or from
# find_package with PIXIE_SYSTEM_GLM; whoever only wants the primitives turns PIXIE_MATH off and does
# not download it.
option(PIXIE_MATH "Build pixie::math, which brings GLM" ON)
option(PIXIE_SYSTEM_GLM "Take GLM from find_package instead of downloading 1.0.3" OFF)

if(PIXIE_MATH)
    if(PIXIE_SYSTEM_GLM)
        find_package(glm CONFIG REQUIRED)
        set(PIXIE_GLM_TARGET glm::glm)
    else()
        include(FetchContent)
        # SOURCE_SUBDIR names a folder without a CMakeLists.txt, so only the headers come: the CMake of
        # GLM would build a library of its own and set the C++ standard of its folder.
        FetchContent_Declare(pixie_glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # 1.0.3
            SOURCE_SUBDIR pixie-headers-only)
        FetchContent_MakeAvailable(pixie_glm)
        add_library(pixie_glm_headers INTERFACE)
        target_include_directories(pixie_glm_headers SYSTEM INTERFACE ${pixie_glm_SOURCE_DIR})
        set(PIXIE_GLM_TARGET pixie_glm_headers)
    endif()

    # The one compiled file of PixieLib: GLM's SIMD inverse of a float mat4 (6.2), which needs a define
    # of its own (math/src/pxMat.cpp). Everything else in pixie::math is headers.
    add_library(pixie_math STATIC ${CMAKE_CURRENT_SOURCE_DIR}/math/src/pxMat.cpp)
    add_library(pixie::math ALIAS pixie_math)
    target_include_directories(pixie_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/math/include)
    target_link_libraries(pixie_math PUBLIC pixie::pixie ${PIXIE_GLM_TARGET})
    target_compile_features(pixie_math PUBLIC cxx_std_20)
    # Every file that links pixie::math sees GLM the same way (6.3): a conversion between precisions
    # is explicit, as P8.5 asks of the one that loses, and a vector starts at zero, as in C#.
    target_compile_definitions(pixie_math PUBLIC GLM_FORCE_EXPLICIT_CTOR GLM_FORCE_CTOR_INIT)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(PIXIE_BUILD_TESTS "Build the tests of PixieLib" ON)
else()
    option(PIXIE_BUILD_TESTS "Build the tests of PixieLib" OFF)
endif()

if(PIXIE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
