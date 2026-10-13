include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

# The game: everything the particles are, as a Project. The player and the C API library link it.
add_library(particles-game STATIC
    src/game/ParticlesProject.cpp
    src/game/ParticlesScene.cpp
)
set_target_properties(particles-game PROPERTIES FOLDER samples/particles)
target_include_directories(particles-game PUBLIC src/game)
target_link_libraries(particles-game PUBLIC Pixie::Runtime)
pixie_target_defaults(particles-game)

# The player: the game in its own window.
add_executable(particles src/player/main.cpp)
set_target_properties(particles PROPERTIES FOLDER samples/particles)
target_link_libraries(particles PRIVATE particles-game pixie_sample_common)
pixie_target_defaults(particles)

# The game for hosts that drive it through the C API, such as the editor: bin/<config>/particles-capi.
pixie_add_capi_library(particles-capi particles-game)
