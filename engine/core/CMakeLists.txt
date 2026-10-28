add_library(pixie_core STATIC
    src/Image.cpp
    src/Log.cpp
    src/RateMeter.cpp
    src/Telemetry.cpp
)
add_library(Pixie::Core ALIAS pixie_core)
set_target_properties(pixie_core PROPERTIES FOLDER engine)
target_include_directories(pixie_core PUBLIC include)
target_link_libraries(pixie_core PUBLIC pixie::pixie pixie::math)
# CPU image decoding uses private stb headers without another link dependency.
target_include_directories(pixie_core SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external/stb)
pixie_target_defaults(pixie_core)
