add_executable(boids
    src/Boid.cpp
    src/BoidsApp.cpp
    src/BoidsScene.cpp
    src/main.cpp
)

set_target_properties(boids PROPERTIES FOLDER samples)

target_link_libraries(boids PRIVATE Pixie::Runtime pixie_sample_common)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(boids)
