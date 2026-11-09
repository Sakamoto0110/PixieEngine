cmake_minimum_required(VERSION 3.25)

# The product version has one owner, version.json: MAJOR.MINOR.PATCH.REVISION (docs/development.md).
file(READ "${CMAKE_CURRENT_LIST_DIR}/version.json" pixie_version_json)
string(JSON pixie_product_version GET "${pixie_version_json}" version)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/version.json")
project(PixieEngine VERSION ${pixie_product_version} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static third-party objects are embedded in the shared engine on ELF platforms.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# A game's project adds the engine with add_subdirectory (cmake/PixieProject.cmake); then the engine
# is not the top-level project, and builds neither its samples nor its tests by default.
# Samples only exercise what the engine already has; turn them off to build the engine alone.
option(PIXIE_BUILD_SAMPLES "Build the samples under samples/" ${PROJECT_IS_TOP_LEVEL})
# Not PIXIE_BUILD_TESTS: PixieLib owns that name for its own tests.
option(PIXIE_ENGINE_BUILD_TESTS "Build the tests under tests/ and register them with CTest" ${PROJECT_IS_TOP_LEVEL})

# Every executable and shared library lands in bin/<config>/ at the repository root, away from the
# generated build tree. Under a game's project, they land where that project says: its own bin/.
set(PIXIE_OUTPUT_ROOT "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "Root for native runtime and shared-library output")
if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PIXIE_OUTPUT_ROOT}/$<CONFIG>)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PIXIE_OUTPUT_ROOT}/$<CONFIG>)
endif()

# Shared C++ boundaries require the same dynamic runtime in every native target.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Visual Studio: group projects into solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(PIXIE_DEVELOPMENT "Build development-only game modules and loading" ON)
option(PIXIE_DEV_CHANNEL "Build the local development channel" ON)
if(PIXIE_DEV_CHANNEL AND NOT PIXIE_DEVELOPMENT)
    message(FATAL_ERROR "PIXIE_DEV_CHANNEL requires PIXIE_DEVELOPMENT; disable both for shipping")
endif()
# Fresh source builds retain their selected module/channel behavior. An existing
# profile is never reinterpreted when those options change in a cached build.
if(PIXIE_DEV_CHANNEL)
    set(pixie_default_sdk_profile local-development)
elseif(PIXIE_DEVELOPMENT)
    set(pixie_default_sdk_profile authoring)
else()
    set(pixie_default_sdk_profile shipping)
endif()
set(PIXIE_SDK_PROFILE "${pixie_default_sdk_profile}" CACHE STRING "Explicit native SDK capability profile")
set_property(CACHE PIXIE_SDK_PROFILE PROPERTY STRINGS authoring shipping local-development)
if(PIXIE_SDK_PROFILE STREQUAL "authoring")
    if(NOT PIXIE_DEVELOPMENT OR PIXIE_DEV_CHANNEL)
        message(FATAL_ERROR "The authoring profile requires PIXIE_DEVELOPMENT=ON and PIXIE_DEV_CHANNEL=OFF")
    endif()
elseif(PIXIE_SDK_PROFILE STREQUAL "shipping")
    if(PIXIE_DEVELOPMENT OR PIXIE_DEV_CHANNEL)
        message(FATAL_ERROR "The shipping profile requires PIXIE_DEVELOPMENT=OFF and PIXIE_DEV_CHANNEL=OFF")
    endif()
elseif(PIXIE_SDK_PROFILE STREQUAL "local-development")
    if(NOT PIXIE_DEVELOPMENT OR NOT PIXIE_DEV_CHANNEL)
        message(FATAL_ERROR "The private local-development profile requires PIXIE_DEVELOPMENT=ON and PIXIE_DEV_CHANNEL=ON")
    endif()
else()
    message(FATAL_ERROR "Unknown PIXIE_SDK_PROFILE: ${PIXIE_SDK_PROFILE}")
endif()
include(cmake/sdk/PixieSdk.cmake)
pixie_sdk_prepare()
include(cmake/PixieTargetDefaults.cmake)
include(cmake/PixieExportProduct.cmake)
add_library(pixie_shared SHARED)
set_target_properties(pixie_shared PROPERTIES OUTPUT_NAME pixie)
add_library(Pixie::Shared ALIAS pixie_shared)
pixie_target_defaults(pixie_shared)
add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(runtime)
add_subdirectory(capi)
if(PIXIE_DEV_CHANNEL)
    add_subdirectory(devchannel)
    target_sources(pixie_shared PRIVATE $<TARGET_OBJECTS:pixie_devchannel>)
    target_include_directories(pixie_shared PUBLIC "${PROJECT_SOURCE_DIR}/devchannel/include")
    target_include_directories(pixie_capi_objects PRIVATE "${PROJECT_SOURCE_DIR}/devchannel/include")
    target_compile_definitions(pixie_shared PUBLIC PIXIE_DEV_CHANNEL)
    target_compile_definitions(pixie_capi_objects PRIVATE PIXIE_DEV_CHANNEL PIXIE_DEV_BUILD)
endif()
foreach(boundary core platform render app runtime)
    target_sources(pixie_shared PRIVATE $<TARGET_OBJECTS:pixie_${boundary}>)
    target_include_directories(pixie_shared PUBLIC $<TARGET_PROPERTY:pixie_${boundary},INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(pixie_${boundary} PRIVATE PIXIE_CORE_BUILD PIXIE_PLATFORM_BUILD
        PIXIE_RENDER_BUILD PIXIE_APP_BUILD PIXIE_RUNTIME_BUILD GLEW_STATIC)
    set_target_properties(pixie_${boundary} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endforeach()
target_sources(pixie_shared PRIVATE $<TARGET_OBJECTS:pixie_capi_objects>)
target_link_libraries(pixie_shared PUBLIC pixie::pixie pixie::math EnTT::EnTT glew_headers
    PRIVATE glew glfw box2d ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(pixie_shared PRIVATE shell32 ole32)
    if(PIXIE_DEV_CHANNEL)
        target_link_libraries(pixie_shared PRIVATE advapi32)
    endif()
endif()
target_include_directories(pixie_shared PUBLIC "${PROJECT_SOURCE_DIR}/capi/include")
if(PIXIE_DEVELOPMENT)
    target_compile_definitions(pixie_shared PUBLIC PIXIE_DEVELOPMENT)
    target_compile_definitions(pixie_capi_objects PRIVATE PIXIE_DEVELOPMENT)
    target_compile_definitions(pixie_app PRIVATE PIXIE_DEVELOPMENT)
endif()
pixie_sdk_attach(pixie_shared)
# pixie_game(): directly linked game code, a player and an optional development module.
include(cmake/PixieGame.cmake)
option(PIXIE_BUILD_TOOLS "Build the native SDK export tool" ${PROJECT_IS_TOP_LEVEL})
if(PIXIE_BUILD_TOOLS)
    # The paired exporter is SDK authoring-tool payload and may link the shipping
    # runtime. Game export still disables tools and admits only its manifest files.
    add_subdirectory(tools/export)
endif()
if(PIXIE_ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(PIXIE_BUILD_SAMPLES)
    add_subdirectory(samples)
    # F5 starts the sample.
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT hello-gl)
endif()
if(PIXIE_ENGINE_BUILD_TESTS AND PIXIE_DEVELOPMENT)
    include(cmake/PixieSnapshots.cmake)
    pixie_add_snapshot_tests()
    if(WIN32 AND TARGET boids AND TARGET flocks AND TARGET particles AND TARGET sparks)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        add_test(NAME deterministic-capture
            COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tests/capture/qualify_capture.py"
                --bin $<TARGET_FILE_DIR:boids> --work "${PROJECT_BINARY_DIR}/tests/capture")
        set_tests_properties(deterministic-capture PROPERTIES RUN_SERIAL TRUE TIMEOUT 180 LABELS capture)
    endif()
endif()

# Dedicated SDK producers opt in after all runtime/player targets are registered.
pixie_sdk_install()

# PixieEngine.slnx at the repository root; see the module for what it shows and why.
include(cmake/PixieRootSolution.cmake)
pixie_write_root_solution()
if(TARGET module-lifetime AND TARGET boids-module)
    add_test(NAME module-lifetime COMMAND module-lifetime
        $<TARGET_FILE:boids-module> $<TARGET_FILE:pixie_shared> $<TARGET_FILE:null-game-module>)
endif()
