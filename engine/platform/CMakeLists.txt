add_library(pixie_platform OBJECT
    src/FrameLimiter.cpp
    src/Input.cpp
    src/Window.cpp
)
if(PIXIE_DEV_CHANNEL)
    target_sources(pixie_platform PRIVATE src/LocalCommandChannel.cpp)
endif()
add_library(pixie_platform_api INTERFACE)
add_library(Pixie::Platform ALIAS pixie_platform_api)
target_link_libraries(pixie_platform_api INTERFACE pixie_shared)
set_target_properties(pixie_platform PROPERTIES FOLDER engine)
target_include_directories(pixie_platform PUBLIC include)
target_link_libraries(pixie_platform PUBLIC pixie_core glfw)
target_compile_definitions(pixie_platform PRIVATE GLFW_INCLUDE_NONE)
pixie_target_defaults(pixie_platform)
