#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/ParticleEmitter.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"

namespace pixie {

void DescribeRuntimeComponents(ComponentTypes& types) {
    types.Add<Transform2D>("Transform2D")
        .Field<&Transform2D::position>("position")
        .Field<&Transform2D::rotation>("rotation")
        .Field<&Transform2D::scale>("scale");
    types.Add<Velocity2D>("Velocity2D")
        .Field<&Velocity2D::linear>("linear")
        .Field<&Velocity2D::angular>("angular");
    types.Add<Acceleration2D>("Acceleration2D").Field<&Acceleration2D::linear>("linear");
    types.Add<Shape>("Shape")
        .Field<&Shape::kind>("kind", {"Triangle", "Circle", "Ring", "Quad"})
        .Field<&Shape::size>("size")
        .Field<&Shape::color>("color");
    types.Add<Attractor>("Attractor")
        .Field<&Attractor::strength>("strength")
        .Field<&Attractor::softening>("softening")
        .Field<&Attractor::radius>("radius");
    types.Add<ParticleEmitter>("ParticleEmitter")
        .Field<&ParticleEmitter::rate>("rate")
        .Field<&ParticleEmitter::lifetime>("lifetime")
        .Field<&ParticleEmitter::speedMin>("speedMin")
        .Field<&ParticleEmitter::speedMax>("speedMax")
        .Field<&ParticleEmitter::spread>("spread");
}

} // namespace pixie
