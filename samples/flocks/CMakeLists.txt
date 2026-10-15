# The flocks sample is a project folder (flocks.pixie and scenes/), with code. Built with the engine,
# it is one of the engine's samples: its player lands in the engine's bin/<config>/ and opens this
# folder (PROJECT_DIR). Opened in the editor, the folder is a project of its own, built like any
# other against the engine the editor runs from, into build/ and bin/ here.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.25)
    project(flocks LANGUAGES C CXX)
    if(NOT IS_DIRECTORY "${PIXIE_ENGINE_DIR}")
        message(FATAL_ERROR "PIXIE_ENGINE_DIR must name PixieEngine's folder; the editor passes it when it builds this project.")
    endif()
    include(${PIXIE_ENGINE_DIR}/cmake/PixieProject.cmake)
endif()

pixie_game(flocks
    SOURCES
        src/game/Flocker.cpp
        src/game/FlocksProject.cpp
        src/game/FlocksScene.cpp
    INCLUDE_DIRS src/game
    PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(flocks-game flocks PROPERTIES FOLDER samples/flocks)
