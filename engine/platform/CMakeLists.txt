add_library(pixie_platform STATIC
    src/FrameLimiter.cpp
    src/Input.cpp
    src/Window.cpp
)
add_library(Pixie::Platform ALIAS pixie_platform)
set_target_properties(pixie_platform PROPERTIES FOLDER engine)
target_include_directories(pixie_platform PUBLIC include)
target_link_libraries(pixie_platform PUBLIC Pixie::Core glfw)
target_compile_definitions(pixie_platform PRIVATE GLFW_INCLUDE_NONE)
pixie_target_defaults(pixie_platform)
