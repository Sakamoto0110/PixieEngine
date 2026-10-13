# The runtime module (docs/modules/runtime/): the World, entities, Behaviours, and update phases.

add_library(pixie_runtime STATIC
    src/AttractorAffector.cpp
    src/InteractionAffector.cpp
    src/ParticleSystem.cpp
    src/Random.cpp
    src/ShapeDrawing.cpp
    src/SpatialGrid.cpp
    src/World.cpp
)
add_library(Pixie::Runtime ALIAS pixie_runtime)
set_target_properties(pixie_runtime PROPERTIES FOLDER engine)

target_include_directories(pixie_runtime PUBLIC include)
target_link_libraries(pixie_runtime PUBLIC Pixie::Engine EnTT::EnTT)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(pixie_runtime)
