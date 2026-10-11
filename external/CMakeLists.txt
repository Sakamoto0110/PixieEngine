# Third-party targets. Sources stay unmodified; see DEPENDENCIES.md.

find_package(OpenGL REQUIRED)

# GLEW: built here as a static library instead of through its own CMake project,
# which also builds a shared library, utilities, and install rules.
add_library(glew STATIC glew/src/glew.c)
target_include_directories(glew SYSTEM PUBLIC glew/include)
target_compile_definitions(glew PUBLIC GLEW_STATIC GLEW_NO_GLU)
target_link_libraries(glew PUBLIC OpenGL::GL)

# GLFW: built through its own CMake project, without docs, tests, or examples.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# stb_easy_font: header-only.
add_library(stb_easy_font INTERFACE)
target_include_directories(stb_easy_font SYSTEM INTERFACE stb)

foreach(target glew glfw update_mappings)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER external)
    endif()
endforeach()
