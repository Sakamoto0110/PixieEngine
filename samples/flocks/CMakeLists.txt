add_executable(flocks
    src/Flocker.cpp
    src/FlocksApp.cpp
    src/FlocksScene.cpp
    src/main.cpp
)

set_target_properties(flocks PROPERTIES FOLDER samples)

target_link_libraries(flocks PRIVATE Pixie::Runtime pixie_sample_common)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(flocks)
