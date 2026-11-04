cmake_minimum_required(VERSION 3.22)
project(pixie_local_gravity_probe LANGUAGES C CXX)

# Standalone qualification target. It does not link or modify the normal runtime.
set(BOX2D_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../external/box2d" "${CMAKE_CURRENT_BINARY_DIR}/box2d")
add_executable(local-gravity-probe main.cpp)
target_compile_features(local-gravity-probe PRIVATE cxx_std_20)
target_link_libraries(local-gravity-probe PRIVATE box2d)
