# The runtime module (docs/modules/runtime/): the World, entities, Behaviours, and update phases.

add_library(pixie_runtime STATIC
    src/AttractorAffector.cpp
    src/InteractionAffector.cpp
    src/ParticleSpawner.cpp
    src/Picking.cpp
    src/ParticleStep.cpp
    src/ProjectApp.cpp
    src/ProjectFile.cpp
    src/ProjectSession.cpp
    src/Random.cpp
    src/RuntimeComponents.cpp
    src/Scene.cpp
    src/SceneProject.cpp
    src/ShapeDrawing.cpp
    src/SpriteDrawing.cpp
    src/SpriteImages.cpp
    src/SpatialGrid.cpp
    src/World.cpp
)
add_library(Pixie::Runtime ALIAS pixie_runtime)
set_target_properties(pixie_runtime PROPERTIES FOLDER engine)

target_include_directories(pixie_runtime PUBLIC include)
target_link_libraries(pixie_runtime PUBLIC Pixie::Engine EnTT::EnTT)
# Scenes and project files are read and written through nlohmann/json; no public header includes it.
target_link_libraries(pixie_runtime PRIVATE nlohmann_json::nlohmann_json)

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(pixie_runtime)
