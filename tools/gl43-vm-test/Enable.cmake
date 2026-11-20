# Explicit CMAKE_PROJECT_PixieEngine_INCLUDE for the isolated VM build.
if(NOT WIN32 OR NOT MSVC OR CMAKE_BINARY_DIR STREQUAL "${PROJECT_SOURCE_DIR}/build")
    message(FATAL_ERROR "The OpenGL 4.3 VM test requires Windows MSVC and a separate build directory")
endif()

# Bind the variant to its own SDK identity, including the exact compatibility code.
foreach(file IN ITEMS Enable.cmake Gl43.hpp Window43.hpp)
    file(SHA256 "${CMAKE_CURRENT_LIST_DIR}/${file}" hash)
    string(APPEND vm_inputs "${hash}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${file}")
endforeach()
string(SHA256 vm_hash "${vm_inputs}")
string(APPEND CMAKE_CXX_FLAGS " /DPIXIE_GL43_VM_TEST_${vm_hash}")

function(pixie_enable_vm_gl43 directory)
    target_compile_options(pixie_render PRIVATE "/FI${directory}/Gl43.hpp")
    target_compile_options(pixie_platform PRIVATE "/FI${directory}/Window43.hpp")
endfunction()

cmake_language(EVAL CODE
    "cmake_language(DEFER CALL pixie_enable_vm_gl43 [[${CMAKE_CURRENT_LIST_DIR}]])")
