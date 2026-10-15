# Samples are separate projects that consume the engine; the engine never depends on them.
add_subdirectory(boids)
add_subdirectory(flocks)
add_subdirectory(hello-gl)
add_subdirectory(particles)
add_subdirectory(shapes)

# The sprites sample is a project folder without code, so it has no CMakeLists.txt of its own: the
# generic player plays it (pixie-player --project samples/sprites), and the generic library opens it.
if(TARGET pixie-scene-capi AND TARGET capi-test)
    add_test(NAME sprites-project
             COMMAND capi-test $<TARGET_FILE:pixie-scene-capi> --open ${CMAKE_CURRENT_SOURCE_DIR}/sprites --scene-only)
endif()
