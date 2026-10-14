# Gives a project folder code of its own, from the engine's templates (templates/project-code/):
#
#   cmake -DPIXIE_PROJECT_DIR=<project folder> -P <engine>/cmake/PixieAddCode.cmake
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
if(EXISTS "${PIXIE_PROJECT_DIR}/CMakeLists.txt")
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
configure_file("${templates}/CMakeLists.txt.in" "${PIXIE_PROJECT_DIR}/CMakeLists.txt" @ONLY)
configure_file("${templates}/Project.hpp.in" "${PIXIE_PROJECT_DIR}/src/${PIXIE_ID}Project.hpp" @ONLY)
configure_file("${templates}/Project.cpp.in" "${PIXIE_PROJECT_DIR}/src/${PIXIE_ID}Project.cpp" @ONLY)
message(STATUS "Added code to ${PIXIE_NAME}: CMakeLists.txt, src/${PIXIE_ID}Project.hpp, src/${PIXIE_ID}Project.cpp")
