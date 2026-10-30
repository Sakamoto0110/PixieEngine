# pixie_game(<name> SOURCES <file>... [INCLUDE_DIRS <dir>...] [PROJECT_DIR <dir>]): a game written
# against the engine, as three targets:
#
# - <name>-game, a static library with the game's code, which defines pixie::CreateProject();
# - <name>, its player, whose main is the engine's (runtime/player/PlayerMain.cpp): it opens the
#   project folder it is in and plays the start scene;
# - <name>-module, a development-only factory and game code, using the engine's C API.
#
# INCLUDE_DIRS are the game library's public include directories; src/ when none are given. The player
# is the Visual Studio startup project of the directory that calls this.
#
# PROJECT_DIR is for a game built in another tree than its project, such as the engine's samples,
# whose players land in the engine's bin/: the player opens that folder unless --project says
# otherwise, and a CTest test opens the project through the game's library (capi-test --open).

function(pixie_game name)
    cmake_parse_arguments(PARSE_ARGV 1 GAME "" "PROJECT_DIR" "SOURCES;INCLUDE_DIRS;ASSETS")
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
    if(GAME_PROJECT_DIR AND PIXIE_DEVELOPMENT)
        target_compile_definitions(${name} PRIVATE "PIXIE_PLAYER_PROJECT_DIR=\"${GAME_PROJECT_DIR}\"")
    endif()
    pixie_export_product(${name} ASSETS ${GAME_ASSETS})

    if(PIXIE_DEVELOPMENT)
        add_library(${name}-module MODULE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../capi/src/GameModuleEntry.cpp)
        target_link_libraries(${name}-module PRIVATE ${name}-game)
        pixie_target_defaults(${name}-module)
        set_target_properties(${name}-module PROPERTIES FOLDER game)
        if(TARGET capi-test)
            add_test(NAME ${name}-module COMMAND capi-test $<TARGET_FILE:pixie_shared> --module $<TARGET_FILE:${name}-module>)
            if(GAME_PROJECT_DIR)
                add_test(NAME ${name}-project COMMAND capi-test $<TARGET_FILE:pixie_shared>
                    --open ${GAME_PROJECT_DIR} --module $<TARGET_FILE:${name}-module>)
            endif()
        endif()
    endif()

    set_target_properties(${name}-game ${name} PROPERTIES FOLDER game)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${name})
endfunction()
