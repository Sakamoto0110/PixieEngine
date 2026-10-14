add_executable(shapes
    src/ShapesApp.cpp
    src/main.cpp
)

set_target_properties(shapes PROPERTIES FOLDER samples)

target_link_libraries(shapes PRIVATE Pixie::Engine)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(shapes)
