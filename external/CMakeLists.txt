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

# stb_image: header-only; the engine compiles its implementation (PNG only) in Image.cpp.
add_library(stb_image INTERFACE)
target_include_directories(stb_image SYSTEM INTERFACE stb)

# PixieLib: built through its own CMake project (pixie::pixie, pixie::events, pixie::math). Its math
# fetches GLM with FetchContent; pointing that content at the vendored GLM means a configure never
# downloads it. SYSTEM keeps warnings from PixieLib's and GLM's headers out of our /W4 /WX targets.
set(FETCHCONTENT_SOURCE_DIR_PIXIE_GLM ${CMAKE_CURRENT_SOURCE_DIR}/glm CACHE PATH
    "GLM for PixieLib: the vendored release" FORCE)
add_subdirectory(pixielib/cpp pixielib SYSTEM)

# EnTT: header-only, used straight from its sources instead of through its own CMake project.
add_library(entt INTERFACE)
add_library(EnTT::EnTT ALIAS entt)
target_include_directories(entt SYSTEM INTERFACE entt/src)
target_compile_features(entt INTERFACE cxx_std_20)

# nlohmann/json: the release's single header, used straight from it.
add_library(nlohmann_json INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
target_include_directories(nlohmann_json SYSTEM INTERFACE nlohmann_json/single_include)

foreach(target glew glfw update_mappings pixie_math)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER external)
    endif()
endforeach()
