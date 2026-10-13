add_executable(particles
    src/ParticlesApp.cpp
    src/ParticlesScene.cpp
    src/main.cpp
)

set_target_properties(particles PROPERTIES FOLDER samples)

target_link_libraries(particles PRIVATE Pixie::Runtime pixie_sample_common)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(particles)
