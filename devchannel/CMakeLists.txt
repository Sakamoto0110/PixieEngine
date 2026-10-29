# Private component of the single shared engine; the adapter reuses its C API operations.
add_library(pixie_devchannel OBJECT src/Dispatcher.cpp src/PlayerChannel.cpp)
target_include_directories(pixie_devchannel PUBLIC include PRIVATE ${PROJECT_SOURCE_DIR}/capi/src ${PROJECT_SOURCE_DIR}/capi/include)
target_link_libraries(pixie_devchannel PUBLIC pixie_runtime nlohmann_json::nlohmann_json)
target_compile_definitions(pixie_devchannel PRIVATE PIXIE_CAPI_BUILD PIXIE_DEVELOPMENT PIXIE_DEV_CHANNEL PIXIE_DEV_BUILD)
target_compile_definitions(pixie_devchannel PRIVATE PIXIE_CORE_BUILD PIXIE_PLATFORM_BUILD PIXIE_RENDER_BUILD PIXIE_APP_BUILD PIXIE_RUNTIME_BUILD GLEW_STATIC)
pixie_target_defaults(pixie_devchannel)
set_target_properties(pixie_devchannel PROPERTIES POSITION_INDEPENDENT_CODE ON FOLDER engine)
