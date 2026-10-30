# The test project-code: a project with code made from the engine's templates, built as a project of
# its own against this engine, and its C API library driven by capi-test, so the templates and
# cmake/PixieProject.cmake cannot rot unseen. Run by CTest:
#
#   cmake -DENGINE=<engine> -DWORK=<folder> -DCONFIG=<config> -DGENERATOR=<generator>
#         -DPLATFORM=<platform> [-DGRAPHICS_RUNTIME=<folder>] -DCAPI_TEST=<capi-test> -P ProjectCodeTest.cmake
#
# The project's name has a space, so its code identifier ("CodeTest") is made, not copied.

cmake_minimum_required(VERSION 3.25)

function(run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        list(JOIN ARGN " " command)
        message(FATAL_ERROR "failed (${result}): ${command}")
    endif()
endfunction()

# Script mode knows no target platform, only the host's, which is the one the engine was built on.
if(CMAKE_HOST_WIN32)
    set(library_pattern "*-module.dll")
    set(executable_suffix ".exe")
    set(generic_library "pixie.dll")
else()
    set(library_pattern "lib*-module.so")
    set(executable_suffix "")
    set(generic_library "libpixie.so")
endif()

set(project "${WORK}/Code Test")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${project}")
file(WRITE "${project}/Code Test.pixie"
     "{\n  \"format\": \"pixie-project\",\n  \"version\": 1,\n  \"name\": \"Code Test\",\n  \"startScene\": \"scenes/main.scene\"\n}\n")
file(MAKE_DIRECTORY "${project}/scenes")
file(WRITE "${project}/scenes/main.scene" [=[
{"format":"pixie-scene","version":1,"seed":1,"entities":[
 {"id":"0123456789abcdef","name":"spinner","components":{
  "Transform2D":{"v":1,"position":[0,0],"rotation":0,"scale":[1,1]},
  "Shape":{"v":1,"kind":"Triangle","size":[24,24]},
  "Spinner":{"v":1,"speed":1.5}}}
]}
]=])

run(${CMAKE_COMMAND} "-DPIXIE_PROJECT_DIR=${project}" -P "${ENGINE}/cmake/PixieAddCode.cmake")
foreach(file CMakeLists.txt src/CodeTestProject.hpp src/CodeTestProject.cpp)
    if(NOT EXISTS "${project}/${file}")
        message(FATAL_ERROR "Add code did not write ${file}")
    endif()
endforeach()
# A second time is refused, and changes nothing.
execute_process(COMMAND ${CMAKE_COMMAND} "-DPIXIE_PROJECT_DIR=${project}" -P "${ENGINE}/cmake/PixieAddCode.cmake"
                RESULT_VARIABLE again OUTPUT_QUIET ERROR_QUIET)
if(again EQUAL 0)
    message(FATAL_ERROR "Add code ran twice on the same project")
endif()

set(platform_args "")
if(PLATFORM)
    set(platform_args -A "${PLATFORM}")
endif()
run(${CMAKE_COMMAND} -S "${project}" -B "${project}/build" -G "${GENERATOR}" ${platform_args} "-DPIXIE_ENGINE_DIR=${ENGINE}")
run(${CMAKE_COMMAND} --build "${project}/build" --config "${CONFIG}" --parallel)

file(GLOB libraries "${project}/bin/${CONFIG}/${library_pattern}")
list(LENGTH libraries count)
if(NOT count EQUAL 1)
    message(FATAL_ERROR "expected one C API library in ${project}/bin/${CONFIG}, found ${count}: ${libraries}")
endif()
if(NOT EXISTS "${project}/bin/${CONFIG}/CodeTest${executable_suffix}")
    message(FATAL_ERROR "the player CodeTest was not built in ${project}/bin/${CONFIG}")
endif()
# The engine's own bin/ is left alone: the game's project builds no generic library.
if(NOT EXISTS "${project}/bin/${CONFIG}/${generic_library}")
    message(FATAL_ERROR "the game's project did not deploy the shared engine")
endif()

run("${CAPI_TEST}" "${project}/bin/${CONFIG}/${generic_library}" --project "${WORK}/opened" --expect-type Spinner --module "${libraries}")
# The player opens its window from the project's bin/, so a software OpenGL provider goes beside it.
if(GRAPHICS_RUNTIME)
    file(GLOB provider "${GRAPHICS_RUNTIME}/*.dll")
    file(COPY ${provider} DESTINATION "${project}/bin/${CONFIG}")
endif()
run("${project}/bin/${CONFIG}/CodeTest${executable_suffix}" --project "${project}" --quit-after 0.1)
