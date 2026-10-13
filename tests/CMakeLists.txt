# Tests run through CTest: ctest --preset vs2026-debug (or -release). Each executable runs the tests
# registered in its sources and exits 1 if any check fails.

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

add_executable(engine-tests
    TestMain.cpp
    engine/CameraTests.cpp
    engine/InputTests.cpp
)
set_target_properties(engine-tests PROPERTIES FOLDER tests)
target_include_directories(engine-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(engine-tests PRIVATE Pixie::Engine)
pixie_target_defaults(engine-tests)
add_test(NAME engine-tests COMMAND engine-tests)

add_executable(runtime-tests
    TestMain.cpp
    runtime/OrderIndependenceTests.cpp
    runtime/RandomTests.cpp
    runtime/SpatialGridTests.cpp
    runtime/WorldTests.cpp
)
set_target_properties(runtime-tests PROPERTIES FOLDER tests)
target_include_directories(runtime-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(runtime-tests PRIVATE Pixie::Runtime)
pixie_target_defaults(runtime-tests)
add_test(NAME runtime-tests COMMAND runtime-tests)
