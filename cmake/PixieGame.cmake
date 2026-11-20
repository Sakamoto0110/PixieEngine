# pixie_game(<name> SOURCES <file>... [INCLUDE_DIRS <dir>...] [PROJECT_DIR <dir>]
#            [ASSETS <project-relative-file>...] [CANDIDATE_AUTHORING]): a game written
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
# PROJECT_DIR provides a source-development fallback when the player finds no project
# beside or above its executable. --project takes precedence. CTest can open that folder
# through the development module. ASSETS declares code-owned inputs for export.
# CANDIDATE_AUTHORING opts that module into the cooperative fresh-Project contract.

function(pixie_game name)
    cmake_parse_arguments(PARSE_ARGV 1 GAME "CANDIDATE_AUTHORING" "PROJECT_DIR" "SOURCES;INCLUDE_DIRS;ASSETS")
    if(NOT GAME_SOURCES)
        message(FATAL_ERROR "pixie_game(${name}) needs SOURCES: the game's .cpp files.")
    endif()
    if(NOT GAME_INCLUDE_DIRS)
        set(GAME_INCLUDE_DIRS src)
    endif()
    if(PIXIE_ENGINE_MODE STREQUAL "PACKAGE")
        set(player_source "${PIXIE_SDK_ROOT}/support/player/PlayerMain.cpp")
        set(module_source "${PIXIE_SDK_ROOT}/support/module/GameModuleEntry.cpp")
        if(NOT EXISTS "${player_source}" OR (PIXIE_DEVELOPMENT AND NOT EXISTS "${module_source}"))
            message(FATAL_ERROR "The selected SDK does not supply its declared player/module support; no source fallback")
        endif()
        if(EXISTS "${PIXIE_SDK_ROOT}/cmake/PixieExportProduct.cmake" AND NOT COMMAND pixie_export_product)
            include("${PIXIE_SDK_ROOT}/cmake/PixieExportProduct.cmake")
        endif()
        if(PIXIE_PROJECT_BUILD_ROLE STREQUAL "shipping-export" AND NOT COMMAND pixie_export_product)
            message(FATAL_ERROR "The selected SDK has no qualified export-product helper; export support is not yet available")
        endif()
    elseif(PIXIE_ENGINE_MODE STREQUAL "SOURCE" OR (NOT DEFINED PIXIE_ENGINE_MODE AND TARGET pixie_shared))
        # A producer/sample source graph is explicit by its existing native target.
        set(player_source "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/player/PlayerMain.cpp")
        set(module_source "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../capi/src/GameModuleEntry.cpp")
    else()
        message(FATAL_ERROR "pixie_game requires an explicit installed package or source-development graph")
    endif()

    add_library(${name}-game STATIC ${GAME_SOURCES})
    set_property(DIRECTORY APPEND PROPERTY PIXIE_GAME_CODE_TARGETS ${name}-game)
    target_include_directories(${name}-game PUBLIC ${GAME_INCLUDE_DIRS})
    target_link_libraries(${name}-game PUBLIC Pixie::Runtime)
    pixie_target_defaults(${name}-game)

    # This file's directory, also when a game's project calls the function.
    add_executable(${name} "${player_source}")
    target_link_libraries(${name} PRIVATE ${name}-game)
    pixie_target_defaults(${name})
    if(GAME_PROJECT_DIR AND PIXIE_DEVELOPMENT AND NOT PIXIE_ENGINE_MODE STREQUAL "PACKAGE")
        target_compile_definitions(${name} PRIVATE "PIXIE_PLAYER_PROJECT_DIR=\"${GAME_PROJECT_DIR}\"")
    endif()
    if(COMMAND pixie_export_product)
        pixie_export_product(${name} ASSETS ${GAME_ASSETS})
    endif()
    if(PIXIE_ENGINE_MODE STREQUAL "PACKAGE")
        pixie_stage_runtime(${name})
    endif()

    if(PIXIE_DEVELOPMENT)
        add_library(${name}-module MODULE "${module_source}")
        target_link_libraries(${name}-module PRIVATE ${name}-game)
        if(GAME_CANDIDATE_AUTHORING)
            target_compile_definitions(${name}-module PRIVATE PIXIE_CANDIDATE_AUTHORING)
        endif()
        pixie_target_defaults(${name}-module)
        set_target_properties(${name}-module PROPERTIES FOLDER game)
        if(PIXIE_ENGINE_MODE STREQUAL "PACKAGE")
            pixie_stage_runtime(${name}-module)
        endif()
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
