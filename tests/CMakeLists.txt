# Tests run through CTest: ctest --preset vs2026-debug (or -release). Each executable runs the tests
# registered in its sources and exits 1 if any check fails.

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

add_executable(engine-tests
    TestMain.cpp
    engine/CameraTests.cpp
    engine/FixedStepperTests.cpp
    engine/InputTests.cpp
)
set_target_properties(engine-tests PROPERTIES FOLDER tests)
target_include_directories(engine-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(engine-tests PRIVATE Pixie::Engine)
pixie_target_defaults(engine-tests)
add_test(NAME engine-tests COMMAND engine-tests)

add_executable(runtime-tests
    TestMain.cpp
    runtime/ComponentTypesTests.cpp
    runtime/OrderIndependenceTests.cpp
    runtime/ParticleTests.cpp
    runtime/RandomTests.cpp
    runtime/SpatialGridTests.cpp
    runtime/WorldTests.cpp
)
set_target_properties(runtime-tests PROPERTIES FOLDER tests)
target_include_directories(runtime-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(runtime-tests PRIVATE Pixie::Runtime)
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
