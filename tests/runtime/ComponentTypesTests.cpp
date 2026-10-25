#include "Check.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cstdint>

using namespace pixie;

TEST(SingletonAdmissionIsGenericAndExistingNondefaultComponentsAreAllowed) {
    struct Unique { double value = 1; };
    struct NeedsValue {
        explicit NeedsValue(double initial) : value(initial) {}
        double value;
    };
    ComponentTypes types;
    types.Add<Unique>("Unique").Singleton().Field<&Unique::value>("value");
    types.Add<NeedsValue>("NeedsValue").Field<&NeedsValue::value>("value");
    World world;
    auto first = world.Create();
    auto second = world.Create();
    const auto& unique = types.All()[0];
    const auto& constructed = types.All()[1];
    CHECK(unique.AdditionError(world, first.Id()).empty());
    first.Add<Unique>();
    CHECK(unique.AdditionError(world, first.Id()).empty());
    CHECK(!unique.AdditionError(world, second.Id()).empty());
    first.Remove<Unique>();
    CHECK(unique.AdditionError(world, second.Id()).empty());
    CHECK(!constructed.AdditionError(world, first.Id()).empty());
    first.Add<NeedsValue>(42.0);
    CHECK(constructed.AdditionError(world, first.Id()).empty());
    CHECK(first.Get<NeedsValue>().value == 42);
    first.Destroy();
    CHECK(!constructed.AdditionError(world, first.Id()).empty());
}

namespace {

enum class Mode : std::uint8_t { First, Second, Third };

struct Everything {
    bool flag = false;
    int count = 0;
    float ratio = 0.0f;
    double amount = 0.0;
    Mode mode = Mode::First;
    pxPoint at;
    pxVec2 direction;
    pxRect area;
    pxColorRgba color;
};

ComponentTypes Describe() {
    ComponentTypes types;
    types.Add<Everything>("Everything")
        .Field<&Everything::flag>("flag")
        .Field<&Everything::count>("count")
        .Field<&Everything::ratio>("ratio")
        .Field<&Everything::amount>("amount")
        .Field<&Everything::mode>("mode", {"First", "Second", "Third"})
        .Field<&Everything::at>("at")
        .Field<&Everything::direction>("direction")
        .Field<&Everything::area>("area")
        .Field<&Everything::color>("color");
    return types;
}

} // namespace

TEST(FieldKindsAreDeducedFromMemberTypes) {
    const ComponentTypes types = Describe();
    const auto& fields = types.All()[0].fields;
    CHECK(fields.size() == 9);
    CHECK(fields[0].kind == FieldKind::Bool && fields[1].kind == FieldKind::Int);
    CHECK(fields[2].kind == FieldKind::Float && fields[3].kind == FieldKind::Double);
    CHECK(fields[4].kind == FieldKind::Enum && fields[4].enumNames.size() == 3);
    CHECK(fields[5].kind == FieldKind::Point && fields[6].kind == FieldKind::Vector);
    CHECK(fields[7].kind == FieldKind::Rect && fields[8].kind == FieldKind::Color);
}

TEST(FieldsWriteAndReadBackThroughValues) {
    const ComponentTypes types = Describe();
    const auto& fields = types.All()[0].fields;
    Everything e;
    const FieldValue values[] = {{1}, {-42}, {0.5}, {3.25}, {2}, {1.5, -2.5}, {0.0, 1.0},
                                 {10, 20, 300, 400}, {255, 128, 0, 64}};
    for (std::size_t i = 0; i < fields.size(); ++i) {
        fields[i].write(&e, values[i]);
    }
    CHECK(e.flag && e.count == -42 && e.ratio == 0.5f && e.amount == 3.25 && e.mode == Mode::Third);
    CHECK(e.at == pxPoint(1.5, -2.5) && e.direction == pxVec2(0.0, 1.0));
    CHECK(e.area == pxRect(10, 20, 300, 400) && e.color == pxColorRgba(255, 128, 0, 64));
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const FieldValue read = fields[i].read(&e);
        CHECK(read.x == values[i].x && read.y == values[i].y && read.z == values[i].z &&
              read.w == values[i].w);
    }
}

TEST(ColorChannelsAreClampedAndRounded) {
    const ComponentTypes types = Describe();
    const FieldDescriptor& color = types.All()[0].fields[8];
    Everything e;
    color.write(&e, {-5.0, 300.0, 127.6, 0.4});
    CHECK(e.color == pxColorRgba(0, 255, 128, 0));
}

TEST(DescriptorsFindComponentsOnEntities) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    const int transform = types.IndexOf("Transform2D");
    CHECK(transform >= 0 && types.IndexOf("NoSuchType") == -1);
    World world;
    Entity with = world.Create();
    with.Add<Transform2D>(Transform2D{pxPoint(4.0, 5.0)});
    Entity without = world.Create();
    const ComponentDescriptor& descriptor = types.All()[static_cast<std::size_t>(transform)];
    void* found = descriptor.find(world.Registry(), with.Id());
    CHECK(found == &with.Get<Transform2D>());
    CHECK(descriptor.find(world.Registry(), without.Id()) == nullptr);
    const FieldValue position = descriptor.fields[0].read(found);
    CHECK(position.x == 4.0 && position.y == 5.0);
    with.Destroy();
    CHECK(descriptor.find(world.Registry(), with.Id()) == nullptr);
}

TEST(MetadataAppliesToTheFieldDeclaredLast) {
    ComponentTypes types;
    types.Add<Everything>("Everything")
        .Field<&Everything::amount>("amount")
        .Label("Amount")
        .Tooltip("How much.")
        .Help("Longer help.")
        .Range(0.0, 10.0, 0.5)
        .Scrub(0.1, ScrubAxis::Vertical)
        .Unit(" kg", 2.0, 1.0)
        .ReadOnly()
        .Field<&Everything::ratio>("ratio")
        .Degrees()
        .Arc("amount")
        .Field<&Everything::count>("count")
        .Order(-1);
    const auto& fields = types.All()[0].fields;
    const FieldMetadata& amount = fields[0].metadata;
    CHECK(amount.label == "Amount" && amount.tooltip == "How much." && amount.help == "Longer help.");
    CHECK(amount.hasRange && amount.rangeMin == 0.0 && amount.rangeMax == 10.0 && amount.rangeStep == 0.5);
    CHECK(amount.scrub == 0.1 && amount.scrubAxis == ScrubAxis::Vertical);
    CHECK(amount.hasUnit && amount.unitSuffix == " kg" && amount.unitFactor == 2.0 && amount.unitOffset == 1.0);
    CHECK(amount.readOnly && amount.order == 0);
    const FieldMetadata& ratio = fields[1].metadata;
    CHECK(ratio.order == 1 && !ratio.readOnly && ratio.tooltip.empty());
    CHECK(ratio.hasUnit && ratio.unitSuffix == "°" && ratio.unitFactor > 57.29 && ratio.unitFactor < 57.3);
    CHECK(ratio.editor == FieldEditor::Arc && ratio.arcSpread == "amount");
    CHECK(fields[2].metadata.order == -1);
}

TEST(EveryRuntimeFieldHasATooltip) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    for (const auto& type : types.All()) {
        for (const auto& field : type.fields) {
            CHECK(!field.metadata.tooltip.empty());
        }
    }
}
