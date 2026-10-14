include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)

# The game: everything the flock is, as a Project. The player and the C API library link it.
add_library(flocks-game STATIC
    src/game/Flocker.cpp
    src/game/FlocksProject.cpp
    src/game/FlocksScene.cpp
)
set_target_properties(flocks-game PROPERTIES FOLDER samples/flocks)
target_include_directories(flocks-game PUBLIC src/game)
target_link_libraries(flocks-game PUBLIC Pixie::Runtime)
pixie_target_defaults(flocks-game)

# The player: the game in its own window.
add_executable(flocks src/player/main.cpp)
set_target_properties(flocks PROPERTIES FOLDER samples/flocks)
target_link_libraries(flocks PRIVATE flocks-game)
pixie_target_defaults(flocks)

# The game for hosts that drive it through the C API, such as the editor: bin/<config>/flocks-capi.
pixie_add_capi_library(flocks-capi flocks-game)
