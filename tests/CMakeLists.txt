# Tests run through CTest: ctest --preset vs2026-debug (or -release). Each executable runs the tests
# registered in its sources and exits 1 if any check fails.

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

add_executable(engine-tests
    TestMain.cpp
    engine/CameraTests.cpp
    engine/FixedStepperTests.cpp
    engine/InputTests.cpp
    engine/TextTests.cpp
)
set_target_properties(engine-tests PROPERTIES FOLDER tests)
target_include_directories(engine-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# GLFW for the hidden window text is drawn in; the font is the particles sample's copy of Inter.
target_link_libraries(engine-tests PRIVATE Pixie::Engine glfw)
target_compile_definitions(engine-tests PRIVATE GLFW_INCLUDE_NONE
    "PIXIE_TEST_FONT=\"${PROJECT_SOURCE_DIR}/samples/particles/fonts/inter-regular.ttf\"")
pixie_target_defaults(engine-tests)
add_test(NAME engine-tests COMMAND engine-tests)

add_executable(runtime-tests
    TestMain.cpp
    runtime/ComponentTypesTests.cpp
    runtime/OrderIndependenceTests.cpp
    runtime/ParticleTests.cpp
    runtime/PathTests.cpp
    runtime/PickingTests.cpp
    runtime/ProjectFileTests.cpp
    runtime/RandomTests.cpp
    runtime/SceneTests.cpp
    runtime/SpatialGridTests.cpp
    runtime/SpriteTests.cpp
    runtime/UserDataTests.cpp
    runtime/WorldTests.cpp
)
set_target_properties(runtime-tests PROPERTIES FOLDER tests)
target_include_directories(runtime-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(runtime-tests PRIVATE Pixie::Runtime nlohmann_json::nlohmann_json)
pixie_target_defaults(runtime-tests)
add_test(NAME runtime-tests COMMAND runtime-tests)

# The C API's test: a C program that loads a game's capi library at run time. Each library
# registers its own CTest test with it (see pixie_add_capi_library in capi/CMakeLists.txt).
add_executable(capi-test capi/CapiTest.c)
set_target_properties(capi-test PROPERTIES FOLDER tests C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_include_directories(capi-test PRIVATE ${PROJECT_SOURCE_DIR}/capi/include)
target_link_libraries(capi-test PRIVATE glfw)
if(MSVC)
    target_compile_options(capi-test PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(capi-test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(capi-test PRIVATE ${CMAKE_DL_LIBS})
endif()

# The generic library runs no game of its own: its test creates a project in a folder of the build
# tree and drives it through creating, editing, saving, playing, and stopping.
if(TARGET pixie-scene-capi)
    add_test(NAME pixie-scene-capi
             COMMAND capi-test $<TARGET_FILE:pixie-scene-capi> --project ${CMAKE_CURRENT_BINARY_DIR}/capi-project)
endif()

# A project with code made from the templates, built as a project of its own against this engine, and
# its library driven by capi-test (see the script). It builds the engine again, so it is the slowest.
add_test(NAME project-code
         COMMAND ${CMAKE_COMMAND} -DENGINE=${PROJECT_SOURCE_DIR} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/project-code
                 -DCONFIG=$<CONFIG> "-DGENERATOR=${CMAKE_GENERATOR}" "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                 -DCAPI_TEST=$<TARGET_FILE:capi-test> -P ${CMAKE_CURRENT_SOURCE_DIR}/project-code/ProjectCodeTest.cmake)
