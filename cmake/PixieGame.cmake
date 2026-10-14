# pixie_game(<name> SOURCES <file>... [INCLUDE_DIRS <dir>...]): a game written against the engine, as
# three targets:
#
# - <name>-game, a static library with the game's code, which defines pixie::CreateProject();
# - <name>, its player, whose main is the engine's (runtime/player/PlayerMain.cpp): it opens the
#   project folder it is in and plays the start scene;
# - <name>-capi, the C API library an editor opens the game's project with.
#
# INCLUDE_DIRS are the game library's public include directories; src/ when none are given. The player
# is the Visual Studio startup project of the directory that calls this.

function(pixie_game name)
    cmake_parse_arguments(PARSE_ARGV 1 GAME "" "" "SOURCES;INCLUDE_DIRS")
    if(NOT GAME_SOURCES)
        message(FATAL_ERROR "pixie_game(${name}) needs SOURCES: the game's .cpp files.")
    endif()
    if(NOT GAME_INCLUDE_DIRS)
        set(GAME_INCLUDE_DIRS src)
    endif()

    add_library(${name}-game STATIC ${GAME_SOURCES})
    target_include_directories(${name}-game PUBLIC ${GAME_INCLUDE_DIRS})
    target_link_libraries(${name}-game PUBLIC Pixie::Runtime)
    pixie_target_defaults(${name}-game)

    # This file's directory, also when a game's project calls the function.
    add_executable(${name} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/player/PlayerMain.cpp)
    target_link_libraries(${name} PRIVATE ${name}-game)
    pixie_target_defaults(${name})

    pixie_add_capi_library(${name}-capi ${name}-game)

    set_target_properties(${name}-game ${name} PROPERTIES FOLDER game)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${name})
endfunction()
