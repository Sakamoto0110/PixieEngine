#include "pixie/runtime/SceneProject.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie_capi.h"
#include <stdexcept>

namespace {
struct WriteProbe { double value = 0; };
class ProbeProject final : public pixie::SceneProject {
public:
    void Describe(pixie::ComponentTypes& types) const override {
        types.Add<WriteProbe>("WriteProbe").Field<&WriteProbe::value>("value")
            .Field<&WriteProbe::value>("locked").ReadOnly();
        // Test-only descriptor fault: the getter succeeds, the setter rejects before mutation.
        auto& descriptor = const_cast<pixie::ComponentDescriptor&>(types.All().back());
        descriptor.fields[0].write = [](void* storage, const pixie::FieldValue& value) {
            if (value.x > 100) throw std::runtime_error("write-probe refused before storage mutation");
            static_cast<WriteProbe*>(storage)->value = value.x;
        };
    }
    void Build(pixie::World& world, pixie::Camera2D&) override { pixie::SceneEntityWith<WriteProbe>(world, "write-probe"); }
};
}
#ifdef _WIN32
#define PROBE_EXPORT __declspec(dllexport)
#else
#define PROBE_EXPORT __attribute__((visibility("default")))
#endif
extern "C" PROBE_EXPORT int pixie_game_api_version() { return PIXIE_CAPI_VERSION; }
extern "C" PROBE_EXPORT pixie::Project* pixie_game_create() { return new ProbeProject(); }
