# C API implementation belongs to the shared engine; game modules contain only game code.
add_library(pixie_capi_objects OBJECT src/Capi.cpp)
target_include_directories(pixie_capi_objects PUBLIC include)
target_compile_definitions(pixie_capi_objects PRIVATE GLEW_STATIC PIXIE_CAPI_BUILD PIXIE_CORE_BUILD
    PIXIE_PLATFORM_BUILD PIXIE_RENDER_BUILD PIXIE_APP_BUILD PIXIE_RUNTIME_BUILD)
target_link_libraries(pixie_capi_objects PUBLIC pixie_runtime)
pixie_target_defaults(pixie_capi_objects)
set_target_properties(pixie_capi_objects PROPERTIES POSITION_INDEPENDENT_CODE ON FOLDER engine)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(pixie-player ${PROJECT_SOURCE_DIR}/runtime/player/PlayerMain.cpp src/SceneGame.cpp)
    target_link_libraries(pixie-player PRIVATE Pixie::Runtime)
    pixie_target_defaults(pixie-player)
    pixie_export_product(pixie-player)
endif()
