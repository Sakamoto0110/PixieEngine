add_executable(pixie-export main.cpp Export.cpp PeImage.cpp Process.cpp)
target_link_libraries(pixie-export PRIVATE Pixie::Runtime nlohmann_json::nlohmann_json)
if(WIN32)
    target_link_libraries(pixie-export PRIVATE bcrypt)
endif()
target_compile_definitions(pixie-export PRIVATE
    "PIXIE_EXPORT_ENGINE_ROOT=\"${PROJECT_SOURCE_DIR}\"" "PIXIE_EXPORT_CMAKE=\"${CMAKE_COMMAND}\""
    "PIXIE_EXPORT_GENERATOR=\"${CMAKE_GENERATOR}\"" "PIXIE_EXPORT_MAKE_PROGRAM=\"${CMAKE_MAKE_PROGRAM}\"")
pixie_target_defaults(pixie-export)
set_target_properties(pixie-export PROPERTIES FOLDER tools)
