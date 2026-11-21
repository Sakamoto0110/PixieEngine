#include "pixie/runtime/SceneProject.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie_capi.h"
#include <map>
#include <memory>
#include <stdexcept>

// Qualification-only controls/audit state. Production contract callbacks must not
// mutate shared state; this fixture deliberately records their execution/cleanup.
namespace {
int fault = 0, nextId = 0, lastBuilt = 0, violations = 0, factoryCalls = 0, unsupportedConstructions = 0;
struct Data { double value; int count; unsigned limit; float ratio; };
struct Unsupported {
    double value = 1;
    Unsupported() { ++unsupportedConstructions; }
};
struct LateSchema { double value; };
struct Probe;
std::map<int, Probe*> instances;
struct Cleanup {
    std::shared_ptr<bool> alive;
    ~Cleanup() { if (!*alive) ++violations; }
};
struct Probe final : pixie::SceneProject {
    int id = ++nextId;
    mutable int state = 0;
    mutable pixie::ComponentTypes* pendingTypes = nullptr;
    std::shared_ptr<bool> alive = std::make_shared<bool>(true);
    Probe() { instances.emplace(id, this); }
    ~Probe() override { *alive = false; instances.erase(id); }
    void Describe(pixie::ComponentTypes& types) const override {
        ++state;
        if (fault == 2) throw std::runtime_error("probe Describe failure");
        types.AddData<Data>("ProbeData").Field<&Data::value>(fault == 3 ? "drift" : "value")
            .Field<&Data::count>("count").Field<&Data::limit>("limit").Field<&Data::ratio>("ratio");
        types.Add<Unsupported>("UnsupportedProbe").Field<&Unsupported::value>("value");
        pendingTypes = fault == 7 ? &types : nullptr;
    }
    void Build(pixie::World& world, pixie::Camera2D&) override {
        ++state;
        lastBuilt = id;
        auto generated = world.Create("generated probe");
        generated.Add<Data>().value = state;
        auto cleanup = std::make_shared<Cleanup>(); cleanup->alive = alive;
        world.AddSystem(pixie::SystemPhase::PostUpdate, "probe-owned callback",
            [this, cleanup](pixie::World&, const pixie::FrameTime&, const pixie::Input&) {
                if (!*cleanup->alive) { ++violations; return; }
                ++state;
            });
        if (fault == 1) throw std::runtime_error("probe Build failure");
        if (fault == 6) {
            auto unsupported = world.Create("unsupported authored output");
            unsupported.Add<Unsupported>();
            pixie::AddToScene(unsupported);
        }
        if (fault == 7 && pendingTypes) pendingTypes->Add<LateSchema>("LateSchema");
        pendingTypes = nullptr;
        if (fault == 8) {
            pixie::ComponentTypes empty;
            const auto report = pixie::ReadScene(world, empty,
                R"({"format":"pixie-scene","version":1,"seed":1,"entities":[]})");
            if (!report.ok) throw std::runtime_error("probe v1 rewrite failed");
        }
        if (fault == 9) {
            for (int i = 0; i < 2; ++i) {
                auto settings = world.Create("duplicate output settings");
                settings.Add<pixie::LightingSettings2D>();
                pixie::AddToScene(settings);
            }
        }
    }
    void Capture(pixie::World&) override {
        state += 1000;
        throw std::runtime_error("contracted reconstruction must not invoke live Capture");
    }
};
}
#ifdef _WIN32
#define PROBE_EXPORT __declspec(dllexport)
#else
#define PROBE_EXPORT __attribute__((visibility("default")))
#endif
std::unique_ptr<pixie::Project> pixie::CreateProject() {
    ++factoryCalls;
    if (fault == 4) throw std::runtime_error("probe factory failure");
    if (fault == 5) return nullptr;
    return std::make_unique<Probe>();
}
extern "C" PROBE_EXPORT int pixie_probe_state(int query, int value) {
    if (query == 0) { fault = value; return 0; }
    if (query == 1) return lastBuilt;
    if (query == 2) { auto found = instances.find(value); return found == instances.end() ? -1 : found->second->state; }
    if (query == 3) return static_cast<int>(instances.size());
    if (query == 4) return violations;
    if (query == 5) return factoryCalls;
    if (query == 6) return unsupportedConstructions;
    return -1;
}
