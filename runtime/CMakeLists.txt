# The runtime module (docs/modules/runtime/): the World, entities, Behaviours, and update phases.

add_library(pixie_runtime OBJECT
    src/LibraryCheck.cpp
    src/CollisionWorld2D.cpp
    src/CharacterController2D.cpp
    src/SceneMovement2D.cpp
    src/MovementGizmos2D.cpp
    src/ColliderEditing2D.cpp
    src/PhysicsDebug2D.cpp
    src/AttractorAffector.cpp
    src/Fonts.cpp
    src/InteractionAffector.cpp
    src/ParticleSpawner.cpp
    src/Culling.cpp
    src/Picking.cpp
    src/ParticleStep.cpp
    src/ParticlePhysics2D.cpp
    src/PathDrawing.cpp
    src/ProjectApp.cpp
    src/ProjectFile.cpp
    src/ProjectSession.cpp
    src/Random.cpp
    src/RuntimeComponents.cpp
    src/Scene.cpp
    src/SceneProject.cpp
    src/ScreenCanvas.cpp
    src/ShapeDrawing.cpp
    src/SpriteDrawing.cpp
    src/SpriteImages.cpp
    src/TextDrawing.cpp
    src/SpatialGrid.cpp
    src/UserData.cpp
    src/World.cpp
)
add_library(pixie_runtime_api INTERFACE)
add_library(Pixie::Runtime ALIAS pixie_runtime_api)
target_link_libraries(pixie_runtime_api INTERFACE pixie_shared)
set_target_properties(pixie_runtime PROPERTIES FOLDER engine)

target_include_directories(pixie_runtime PUBLIC include)
target_link_libraries(pixie_runtime PUBLIC pixie_app EnTT::EnTT)
# Scenes and project files are read and written through nlohmann/json; no public header includes it.
target_link_libraries(pixie_runtime PRIVATE nlohmann_json::nlohmann_json box2d)
# The user data folder is found through the shell's known folders on Windows.
if(WIN32)
    target_link_libraries(pixie_runtime PRIVATE shell32 ole32)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/PixieTargetDefaults.cmake)
pixie_target_defaults(pixie_runtime)
