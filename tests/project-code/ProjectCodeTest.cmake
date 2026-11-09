# The test project-code: a project with code made from the engine's templates, built as a project of
# its own against this engine, and its C API library driven by capi-test, so the templates and
# cmake/PixieProject.cmake cannot rot unseen. Run by CTest:
#
#   cmake -DENGINE=<engine> -DWORK=<folder> -DCONFIG=<config> -DGENERATOR=<generator>
#         -DPLATFORM=<platform> [-DGRAPHICS_RUNTIME=<folder>] -DCAPI_TEST=<capi-test> -P ProjectCodeTest.cmake
#   For an installed run replace ENGINE with -DSDK_ROOT=<package> -DSDK_PROFILE=authoring.
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

if(NOT IS_ABSOLUTE "${WORK}" OR (DEFINED ENGINE AND DEFINED SDK_ROOT))
    message(FATAL_ERROR "Supply an absolute test work directory and exactly one SDK_ROOT or ENGINE")
endif()
if(DEFINED SDK_ROOT)
    if(NOT IS_ABSOLUTE "${SDK_ROOT}" OR NOT SDK_PROFILE MATCHES "^(authoring|local-development)$")
        message(FATAL_ERROR "Installed project-code requires an absolute SDK_ROOT and an admitted authoring SDK_PROFILE")
    endif()
    set(helper_root "${SDK_ROOT}")
    set(selection "-DPIXIE_SDK_ROOT=${SDK_ROOT}" "-DPIXIE_SDK_PROFILE=${SDK_PROFILE}")
    set(generator_args ${selection} -DPIXIE_PROJECT_ADOPT_SDK=ON -DPIXIE_GENERATED_CANDIDATE_AUTHORING=ON)
    set(configure_args ${selection})
elseif(IS_ABSOLUTE "${ENGINE}")
    set(helper_root "${ENGINE}")
    set(generator_args -DPIXIE_ENGINE_MODE=SOURCE)
    set(configure_args -DPIXIE_ENGINE_MODE=SOURCE "-DPIXIE_ENGINE_DIR=${ENGINE}")
else()
    message(FATAL_ERROR "Source project-code requires an explicit absolute ENGINE")
endif()
if(GENERATOR MATCHES "Visual Studio|Multi-Config|Xcode")
    list(APPEND configure_args "-DCMAKE_CONFIGURATION_TYPES=${CONFIG}")
else()
    list(APPEND configure_args "-DCMAKE_BUILD_TYPE=${CONFIG}")
endif()
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef run_id)
set(work "${WORK}/project-code-${run_id}")
set(project "${work}/Code Test")
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

run("${CMAKE_COMMAND}" "-DPIXIE_PROJECT_DIR=${project}" ${generator_args} -P "${helper_root}/cmake/PixieAddCode.cmake")
foreach(file CMakeLists.txt src/CodeTestProject.hpp src/CodeTestProject.cpp)
    if(NOT EXISTS "${project}/${file}")
        message(FATAL_ERROR "Add code did not write ${file}")
    endif()
endforeach()
# A second time is refused, and changes nothing.
execute_process(COMMAND "${CMAKE_COMMAND}" "-DPIXIE_PROJECT_DIR=${project}" ${generator_args} -P "${helper_root}/cmake/PixieAddCode.cmake"
                RESULT_VARIABLE again OUTPUT_QUIET ERROR_QUIET)
if(again EQUAL 0)
    message(FATAL_ERROR "Add code ran twice on the same project")
endif()

set(platform_args "")
if(PLATFORM)
    set(platform_args -A "${PLATFORM}")
endif()
run("${CMAKE_COMMAND}" -S "${project}" -B "${project}/build" -G "${GENERATOR}" ${platform_args} ${configure_args})
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

run("${CAPI_TEST}" "${project}/bin/${CONFIG}/${generic_library}" --project "${work}/opened" --expect-type Spinner --module "${libraries}")
# The optional Mesa provider goes beside the player without copying an unrelated
# runtime or game module from the directory that supplied the provider.
set(graphics_environment "")
if(GRAPHICS_RUNTIME)
    if(NOT EXISTS "${GRAPHICS_RUNTIME}/opengl32.dll")
        message(FATAL_ERROR "GRAPHICS_RUNTIME must supply its opengl32.dll")
    endif()
    foreach(file opengl32.dll libgallium_wgl.dll libglapi.dll)
        if(EXISTS "${GRAPHICS_RUNTIME}/${file}")
            file(COPY "${GRAPHICS_RUNTIME}/${file}" DESTINATION "${project}/bin/${CONFIG}")
        endif()
    endforeach()
    set(graphics_environment "${CMAKE_COMMAND}" -E env GALLIUM_DRIVER=llvmpipe LIBGL_ALWAYS_SOFTWARE=true)
endif()
run(${graphics_environment} "${project}/bin/${CONFIG}/CodeTest${executable_suffix}" --project "${project}" --quit-after 0.1)
