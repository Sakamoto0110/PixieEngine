#include "LightLabProject.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"

using namespace pixie;

void LightLabProject::Describe(ComponentTypes& types) const {
    types.Add<Door2D>("Door2D")
        .Field<&Door2D::velocity>("velocity").Tooltip("Kinematic velocity in world units/s; it reverses every period.").Scrub(1)
        .Field<&Door2D::period>("period").Tooltip("Seconds of simulated time between reversals.").Range(0.1, 60).Unit(" s");
}

void LightLabProject::Build(World& world, Camera2D& /*camera*/) {
    doors_.clear();
    // Before the scene physics step, so the solve that follows uses this tick's velocity.
    world.AddSystem(SystemPhase::PreUpdate, "light-lab.doors", [this](World& w, const FrameTime& time, const Input&) {
        auto* physics = ScenePhysicsWorld2D(w);
        if (!physics) return;
        for (auto [id, door] : w.Registry().view<const Door2D>().each()) {
            const auto handle = SceneBody2D(w, id);
            if (!handle.value || physics->BodyType(handle) != BodyType2D::Kinematic || !(door.period > 0)) continue;
            auto& motion = doors_[id];
            if ((motion.timer += time.delta) >= door.period) { motion.timer -= door.period; motion.sign = -motion.sign; }
            physics->SetBodyVelocity(handle, door.velocity * motion.sign, 0.0);
        }
    });
}

namespace pixie {
std::unique_ptr<Project> CreateProject() { return std::make_unique<LightLabProject>(); }
}
