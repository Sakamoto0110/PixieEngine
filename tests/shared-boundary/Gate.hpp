#pragma once
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Project.hpp"

// Defined by the module fixture, visible in both translation domains to test typed access.
namespace boundary_fixture {
struct Component { double value = 0; };
using Factory = std::unique_ptr<pixie::Project> (*)();
}

#ifdef BOUNDARY_ENGINE_BUILD
#define BOUNDARY_API __declspec(dllexport)
#else
#define BOUNDARY_API __declspec(dllimport)
#endif
extern "C" {
BOUNDARY_API void boundary_factory(boundary_fixture::Factory factory);
BOUNDARY_API pixie::World* boundary_world_create();
BOUNDARY_API void boundary_world_destroy(pixie::World* world);
BOUNDARY_API bool boundary_world_check(pixie::World* world);
BOUNDARY_API unsigned boundary_hash();
BOUNDARY_API unsigned boundary_index();
}
