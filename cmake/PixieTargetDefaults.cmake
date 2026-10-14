# Compiler settings shared by PixieEngine's own targets and by games built with pixie_game (not by
# third-party code). The C++ runtime is linked statically, as everywhere in the engine, so a game's
# targets link with the engine's libraries whatever the game's project sets.
function(pixie_target_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()
