# Compiler settings shared by PixieEngine's own targets and by games built with pixie_game (not by
# third-party code). Every native target uses the shared C++ runtime (/MD or /MDd).

# Code is generated for AVX2 on x86-64, with FMA and F16C: the particle loops and the compiler's
# auto-vectorizer use 256-bit registers. The binaries then need a CPU with AVX2 (Intel since 2013,
# AMD since 2015); turn this off to build for an older one.
option(PIXIE_AVX2 "Generate AVX2, FMA and F16C code for x86-64" ON)

function(pixie_target_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
        if(CMAKE_GENERATOR MATCHES "Visual Studio")
            target_compile_options(${target} PRIVATE /MP)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    if(PIXIE_AVX2 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|amd64)$")
        if(MSVC)
            target_compile_options(${target} PRIVATE /arch:AVX2)
        else()
            target_compile_options(${target} PRIVATE -mavx2 -mfma -mf16c)
        endif()
    endif()
endfunction()
