include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

# The game: everything the boids are, as a Project. The player and the C API library link it.
add_library(boids-game STATIC
    src/game/Boid.cpp
    src/game/BoidsProject.cpp
    src/game/BoidsScene.cpp
)
set_target_properties(boids-game PROPERTIES FOLDER samples/boids)
target_include_directories(boids-game PUBLIC src/game)
target_link_libraries(boids-game PUBLIC Pixie::Runtime)
pixie_target_defaults(boids-game)

# The player: the game in its own window.
add_executable(boids src/player/main.cpp)
set_target_properties(boids PROPERTIES FOLDER samples/boids)
target_link_libraries(boids PRIVATE boids-game pixie_sample_common)
pixie_target_defaults(boids)

# The game for hosts that drive it through the C API, such as the editor: bin/<config>/boids-capi.
pixie_add_capi_library(boids-capi boids-game)
