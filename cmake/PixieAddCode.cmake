# Gives a project folder code of its own, from the engine's templates (templates/project-code/):
#
#   cmake -DPIXIE_PROJECT_DIR=<project folder> -DPIXIE_SDK_ROOT=<selected SDK>
#         -DPIXIE_SDK_PROFILE=authoring -DPIXIE_PROJECT_ADOPT_SDK=ON
#         -P <selected SDK>/cmake/PixieAddCode.cmake
# Explicit Source development uses -DPIXIE_ENGINE_MODE=SOURCE and this source script.
#
# It writes CMakeLists.txt and src/<Id>Project.{hpp,cpp}, where <Id> is the project's name as a C++
# identifier ("my game" is MyGame), read from the folder's .pixie file. It refuses a folder that is not
# a project, or that already has a CMakeLists.txt, and writes nothing then. The editor's Add code runs
# it, and so does a test, so both make the same code.

cmake_minimum_required(VERSION 3.25)

if(NOT IS_DIRECTORY "${PIXIE_PROJECT_DIR}")
    message(FATAL_ERROR "PIXIE_PROJECT_DIR must name a project folder (it is \"${PIXIE_PROJECT_DIR}\").")
endif()
file(GLOB project_files "${PIXIE_PROJECT_DIR}/*.pixie")
list(LENGTH project_files count)
if(NOT count EQUAL 1)
    message(FATAL_ERROR "${PIXIE_PROJECT_DIR} must hold exactly one .pixie file; it holds ${count}.")
endif()
if(EXISTS "${PIXIE_PROJECT_DIR}/CMakeLists.txt" OR IS_SYMLINK "${PIXIE_PROJECT_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "${PIXIE_PROJECT_DIR} already has code: it has a CMakeLists.txt.")
endif()

file(READ "${project_files}" project_json)
string(JSON PIXIE_NAME ERROR_VARIABLE no_name GET "${project_json}" name)
if(no_name)
    get_filename_component(PIXIE_NAME "${project_files}" NAME_WE)
endif()

# The name's letters and digits, each word starting with a capital; a leading digit gets "Game".
string(REGEX MATCHALL "[A-Za-z0-9]+" words "${PIXIE_NAME}")
set(PIXIE_ID "")
foreach(word IN LISTS words)
    string(SUBSTRING "${word}" 0 1 first)
    string(SUBSTRING "${word}" 1 -1 rest)
    string(TOUPPER "${first}" first)
    string(APPEND PIXIE_ID "${first}${rest}")
endforeach()
if(PIXIE_ID STREQUAL "" OR PIXIE_ID MATCHES "^[0-9]")
    set(PIXIE_ID "Game${PIXIE_ID}")
endif()
# Quotes and backslashes would end the name's string literal in the code.
string(REPLACE "\\" "\\\\" PIXIE_NAME_LITERAL "${PIXIE_NAME}")
string(REPLACE "\"" "\\\"" PIXIE_NAME_LITERAL "${PIXIE_NAME_LITERAL}")

set(templates "${CMAKE_CURRENT_LIST_DIR}/../templates/project-code")
if(NOT DEFINED PIXIE_ENGINE_MODE)
    set(PIXIE_ENGINE_MODE PACKAGE)
endif()
if(NOT DEFINED PIXIE_GENERATED_CANDIDATE_AUTHORING)
    set(PIXIE_GENERATED_CANDIDATE_AUTHORING OFF)
endif()
if(NOT PIXIE_GENERATED_CANDIDATE_AUTHORING MATCHES "^(ON|OFF)$")
    message(FATAL_ERROR "PIXIE_GENERATED_CANDIDATE_AUTHORING must be explicit ON/OFF")
endif()
set(PIXIE_GENERATED_CANDIDATE_OPTION "")
set(write_requirement FALSE)
if(PIXIE_ENGINE_MODE STREQUAL "PACKAGE")
    include("${CMAKE_CURRENT_LIST_DIR}/PixieProject.cmake")
    pixie_prepare_project_requirement("${PIXIE_PROJECT_DIR}" requirement write_requirement)
    set(cmake_template "${templates}/sdk/CMakeLists.txt.in")
    set(PIXIE_GENERATOR_FAMILY sdk-v1)
    if(PIXIE_GENERATED_CANDIDATE_AUTHORING STREQUAL "ON")
        set(PIXIE_GENERATED_CANDIDATE_OPTION CANDIDATE_AUTHORING)
    endif()
elseif(PIXIE_ENGINE_MODE STREQUAL "SOURCE")
    if(PIXIE_GENERATED_CANDIDATE_AUTHORING STREQUAL "ON")
        message(FATAL_ERROR "Candidate opt-in is only emitted by the explicitly selected new SDK generator family")
    endif()
    set(cmake_template "${templates}/CMakeLists.txt.in")
    set(PIXIE_GENERATOR_FAMILY registered-v1)
else()
    message(FATAL_ERROR "PIXIE_ENGINE_MODE must be PACKAGE or explicit SOURCE")
endif()

# Preflight every destination before the first write. Existing source is never overwritten.
set(destinations CMakeLists.txt src/${PIXIE_ID}Project.hpp src/${PIXIE_ID}Project.cpp
    src/PixieComponents.generated.hpp src/PixieComponents.generated.cpp
    .pixie-code/Components.cmake .pixie-code/manifest.json)
if(write_requirement)
    list(APPEND destinations .pixie-code/sdk.json)
endif()
foreach(destination IN LISTS destinations)
    if(EXISTS "${PIXIE_PROJECT_DIR}/${destination}" OR IS_SYMLINK "${PIXIE_PROJECT_DIR}/${destination}")
        message(FATAL_ERROR "Refusing to overwrite existing project code: ${destination}")
    endif()
endforeach()
foreach(directory src .pixie-code)
    if(EXISTS "${PIXIE_PROJECT_DIR}/${directory}" AND NOT IS_DIRECTORY "${PIXIE_PROJECT_DIR}/${directory}")
        message(FATAL_ERROR "A generated code directory is an existing file: ${directory}")
    endif()
    if(IS_SYMLINK "${PIXIE_PROJECT_DIR}/${directory}")
        message(FATAL_ERROR "Generated code cannot be written through a linked directory: ${directory}")
    endif()
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/PixieComponentRegistry.cmake")
pixie_validate_generator_templates("${PIXIE_GENERATOR_FAMILY}")
configure_file("${cmake_template}" "${PIXIE_PROJECT_DIR}/CMakeLists.txt" @ONLY)
configure_file("${templates}/Project.hpp.in" "${PIXIE_PROJECT_DIR}/src/${PIXIE_ID}Project.hpp" @ONLY)
configure_file("${templates}/Project.cpp.in" "${PIXIE_PROJECT_DIR}/src/${PIXIE_ID}Project.cpp" @ONLY)
pixie_initialize_component_registry("${PIXIE_PROJECT_DIR}" "${PIXIE_ID}")
if(write_requirement)
    file(WRITE "${PIXIE_PROJECT_DIR}/.pixie-code/sdk.json" "${requirement}")
endif()
message(STATUS "Added project code, explicit component registry and CMake wiring to ${PIXIE_NAME}.")
