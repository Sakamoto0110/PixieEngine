# Tests run through CTest: ctest --preset vs2026-debug (or -release). Each executable runs the tests
# registered in its sources and exits 1 if any check fails.

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

if(PIXIE_DEV_CHANNEL)
    add_library(channel-write-probe-module MODULE editor/channel/NativeWriteProbe.cpp)
    target_link_libraries(channel-write-probe-module PRIVATE Pixie::Runtime)
    pixie_target_defaults(channel-write-probe-module)
    set_target_properties(channel-write-probe-module PROPERTIES FOLDER tests)
    add_executable(devchannel-tests devchannel/DispatcherTests.cpp)
    if(WIN32)
        target_sources(devchannel-tests PRIVATE devchannel/LocalChannelTests.cpp)
        target_link_libraries(devchannel-tests PRIVATE advapi32)
    endif()
    target_include_directories(devchannel-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(devchannel-tests PRIVATE Pixie::Shared glfw nlohmann_json::nlohmann_json)
    pixie_target_defaults(devchannel-tests)
    set_target_properties(devchannel-tests PROPERTIES FOLDER tests)
    add_test(NAME devchannel-tests COMMAND devchannel-tests)
    set_tests_properties(devchannel-tests PROPERTIES RUN_SERIAL TRUE TIMEOUT 60 LABELS devchannel)
endif()

add_executable(engine-tests
    ${PROJECT_SOURCE_DIR}/engine/app/src/DebugConsole.cpp
    ${PROJECT_SOURCE_DIR}/engine/app/src/DebugOverlay.cpp
    TestMain.cpp
    engine/CameraTests.cpp
    engine/ViewTests.cpp
    engine/FixedStepperTests.cpp
    engine/DeterministicCaptureTests.cpp
    engine/InputTests.cpp
    engine/DebugConsoleTests.cpp
    engine/DebugOverlayTests.cpp
    engine/PlayerDebugTests.cpp
    engine/TextTests.cpp
    engine/TelemetryTests.cpp
)
set_target_properties(engine-tests PROPERTIES FOLDER tests)
target_include_directories(engine-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/engine/app/src)
# GLFW for the hidden window text is drawn in; the font is the particles sample's copy of Inter.
target_link_libraries(engine-tests PRIVATE Pixie::Engine glfw stb_easy_font)
target_compile_definitions(engine-tests PRIVATE GLFW_INCLUDE_NONE
    "PIXIE_TEST_FONT=\"${PROJECT_SOURCE_DIR}/samples/particles/fonts/inter-regular.ttf\"")
pixie_target_defaults(engine-tests)
add_test(NAME engine-tests COMMAND engine-tests)

add_executable(runtime-tests
    benchmarks/PhysicsWorkload2D.cpp
    runtime/PhysicsWorkloadTests.cpp
    runtime/ColliderEditingTests.cpp
    runtime/PhysicsDebugTests.cpp
    runtime/PhysicsEventsTests.cpp
    runtime/BodyQualificationTests.cpp
    runtime/RigidBodyTests.cpp
    runtime/CollisionMovementTests.cpp
    runtime/CharacterCouplingTests.cpp
    runtime/SceneMovementTests.cpp
    runtime/MotorGravityTests.cpp
    TestMain.cpp
    runtime/ComponentTypesTests.cpp
    runtime/CompoundAuthoringTests.cpp
    runtime/CollisionLayersTests.cpp
    runtime/OrderIndependenceTests.cpp
    runtime/ParticleTests.cpp
    runtime/ParticlePhysicsTests.cpp
    runtime/PathTests.cpp
    runtime/CullingTests.cpp
    runtime/PickingTests.cpp
    runtime/ProjectFileTests.cpp
    runtime/RandomTests.cpp
    runtime/SceneTests.cpp
    runtime/SpatialGridTests.cpp
    runtime/SpriteTests.cpp
    runtime/TextTests.cpp
    runtime/UserDataTests.cpp
    runtime/WorldTests.cpp
    runtime/TelemetryTests.cpp
)
set_target_properties(runtime-tests PROPERTIES FOLDER tests)
target_include_directories(runtime-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(runtime-tests PRIVATE Pixie::Runtime nlohmann_json::nlohmann_json)
target_compile_definitions(runtime-tests PRIVATE
    "PIXIE_TEST_FONT=\"${PROJECT_SOURCE_DIR}/samples/particles/fonts/inter-regular.ttf\""
    "PIXIE_TEST_MOVEMENT_V1_SCENE=\"${CMAKE_CURRENT_SOURCE_DIR}/runtime/fixtures/movement-lab-v1.scene\"")
pixie_target_defaults(runtime-tests)
add_test(NAME runtime-tests COMMAND runtime-tests)

if(PIXIE_DEVELOPMENT)
    add_library(null-game-module MODULE capi/NullModule.c)
    target_include_directories(null-game-module PRIVATE ${PROJECT_SOURCE_DIR}/capi/include)
    add_executable(module-lifetime capi/ModuleLifetime.c)
    target_link_libraries(module-lifetime PRIVATE Pixie::Shared glfw)
    if(MSVC)
        target_compile_options(null-game-module PRIVATE /W4 /WX)
        target_compile_options(module-lifetime PRIVATE /W4 /WX)
    endif()
endif()



# Explicit headless qualification tool, not an implicit CTest performance budget.
add_executable(physics-benchmark benchmarks/PhysicsTelemetry.cpp benchmarks/PhysicsWorkload2D.cpp)
set_target_properties(physics-benchmark PROPERTIES FOLDER tests)
target_link_libraries(physics-benchmark PRIVATE Pixie::Runtime)
pixie_target_defaults(physics-benchmark)

# The C API's test: a C program that loads a game's capi library at run time. Each library
# registers its own CTest test with it (see pixie_add_capi_library in capi/CMakeLists.txt).
add_executable(capi-test capi/CapiTest.c)
set_target_properties(capi-test PROPERTIES FOLDER tests C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_include_directories(capi-test PRIVATE ${PROJECT_SOURCE_DIR}/capi/include)
target_link_libraries(capi-test PRIVATE glfw)
# A font for the project the test writes: the particles sample's copy of Inter.
target_compile_definitions(capi-test PRIVATE
    "PIXIE_TEST_FONT=\"${PROJECT_SOURCE_DIR}/samples/particles/fonts/inter-regular.ttf\"")
if(MSVC)
    target_compile_options(capi-test PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(capi-test PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(capi-test PRIVATE ${CMAKE_DL_LIBS})
endif()

# The generic library runs no game of its own: its test creates a project in a folder of the build
# tree and drives it through creating, editing, saving, playing, and stopping.
if(TARGET pixie_shared)
    add_test(NAME pixie-scene-capi
             COMMAND capi-test $<TARGET_FILE:pixie_shared> --project ${CMAKE_CURRENT_BINARY_DIR}/capi-project)
    add_test(NAME compound-authoring
             COMMAND capi-test $<TARGET_FILE:pixie_shared> --compounds ${CMAKE_CURRENT_BINARY_DIR}/compound-project)
    add_test(NAME collision-layers
             COMMAND capi-test $<TARGET_FILE:pixie_shared> --layers ${CMAKE_CURRENT_BINARY_DIR}/layer-project)
    add_test(NAME body-authoring
             COMMAND capi-test $<TARGET_FILE:pixie_shared> --bodies ${CMAKE_CURRENT_BINARY_DIR}/body-project)
    add_test(NAME movement-authoring
             COMMAND capi-test $<TARGET_FILE:pixie_shared> --movement ${CMAKE_CURRENT_BINARY_DIR}/movement-project)
endif()

# A project with code made from the templates, built as a project of its own against this engine, and
# its library driven by capi-test (see the script). It builds the engine again, so it is the slowest.
add_test(NAME project-code
         COMMAND ${CMAKE_COMMAND} -DENGINE=${PROJECT_SOURCE_DIR} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/project-code
                 -DCONFIG=$<CONFIG> "-DGENERATOR=${CMAKE_GENERATOR}" "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                 -DCAPI_TEST=$<TARGET_FILE:capi-test> -P ${CMAKE_CURRENT_SOURCE_DIR}/project-code/ProjectCodeTest.cmake)

if(WIN32 AND TARGET pixie-export)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(PIXIE_EXPORT_TEST_GRAPHICS_RUNTIME "" CACHE PATH "Optional software OpenGL provider for export qualification only")
    set(export_graphics_args)
    if(PIXIE_EXPORT_TEST_GRAPHICS_RUNTIME)
        list(APPEND export_graphics_args --graphics-runtime "${PIXIE_EXPORT_TEST_GRAPHICS_RUNTIME}")
    endif()
    add_test(NAME game-export COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/export/qualify_export.py"
        --exporter $<TARGET_FILE:pixie-export> --engine "${PROJECT_SOURCE_DIR}"
        --work "${CMAKE_CURRENT_BINARY_DIR}/export" --cmake "${CMAKE_COMMAND}" --generator "${CMAKE_GENERATOR}"
        ${export_graphics_args})
    set_tests_properties(game-export PROPERTIES RUN_SERIAL TRUE TIMEOUT 600 LABELS export)
endif()
