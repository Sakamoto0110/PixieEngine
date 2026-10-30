add_library(pixie_render OBJECT
    src/2d/OverlayDrawing.cpp
    src/2d/Camera2D.cpp
    src/2d/Font.cpp
    src/2d/ParticleRenderer.cpp
    src/2d/PathRenderer.cpp
    src/2d/ShapeRenderer.cpp
    src/2d/SpriteRenderer.cpp
    src/2d/TextRenderer.cpp
    src/gl/GlBuffer.cpp
    src/gl/GlDebugOutput.cpp
    src/gl/GlLoader.cpp
    src/gl/GlVertexArray.cpp
    src/gl/ShaderProgram.cpp
    src/gl/Texture.cpp
    src/RenderTarget.cpp
    src/Screenshot.cpp
)
add_library(pixie_render_api INTERFACE)
add_library(Pixie::Render ALIAS pixie_render_api)
target_link_libraries(pixie_render_api INTERFACE pixie_shared)
set_target_properties(pixie_render PROPERTIES FOLDER engine)
target_include_directories(pixie_render PUBLIC include)
target_link_libraries(pixie_render PUBLIC pixie_core glew)
target_link_libraries(pixie_render PRIVATE stb_truetype)
pixie_target_defaults(pixie_render)
