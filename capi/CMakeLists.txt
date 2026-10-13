# The capi module (docs/modules/capi/): the C API, built into one shared library per game.
#
# The API's code is an object library, so every object file lands in each game's library (a static
# library would leave out the exported functions, which nothing in the library calls).

add_library(pixie_capi_objects OBJECT src/Capi.cpp)
set_target_properties(pixie_capi_objects PROPERTIES FOLDER engine)
target_include_directories(pixie_capi_objects PUBLIC include)
target_compile_definitions(pixie_capi_objects PRIVATE PIXIE_CAPI_BUILD)
target_link_libraries(pixie_capi_objects PUBLIC Pixie::Runtime)
include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(pixie_capi_objects)

# pixie_add_capi_library(<name> <game>): the shared library <name> (bin/<config>/<name>.dll) that
# hosts the game library <game>, whose pixie::CreateProject the API calls. When the capi-test
# program exists, a CTest test drives the library with it.
function(pixie_add_capi_library name game)
    add_library(${name} SHARED $<TARGET_OBJECTS:pixie_capi_objects>)
    set_target_properties(${name} PROPERTIES FOLDER capi)
    target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR}/capi/include)
    target_link_libraries(${name} PRIVATE ${game} Pixie::Runtime)
    pixie_target_defaults(${name})
    if(TARGET capi-test)
        add_test(NAME ${name} COMMAND capi-test $<TARGET_FILE:${name}>)
    endif()
endfunction()
