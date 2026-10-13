# Code shared by the samples (command-line options); it uses only the engine, like the
# samples do.

add_library(pixie_sample_common STATIC
    src/SampleOptions.cpp
)
set_target_properties(pixie_sample_common PROPERTIES FOLDER samples)

target_include_directories(pixie_sample_common PUBLIC include)
target_link_libraries(pixie_sample_common PUBLIC Pixie::Engine)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(pixie_sample_common)
