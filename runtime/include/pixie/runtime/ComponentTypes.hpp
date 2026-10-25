#pragma once

#include "pixie/runtime/World.hpp"

#include <entt/entity/registry.hpp>

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pixie {

// Compatibility data belongs to the list item, so copy/remove/reorder operations keep unknown
// scene fields attached to their original value. It is serialization data, never runtime state.
template<typename T> struct ComponentRecord {
    T value{};
    std::string unknownFields;
    bool operator==(const ComponentRecord&) const = default;
};

// What a reflected field holds; every kind travels as a FieldValue.
enum class FieldKind : std::uint8_t {
    Bool,    // x: 0 or 1
    Int,     // x
    Float,   // x
    Double,  // x
    Enum,    // x: the enumerator's value; FieldDescriptor::enumNames names 0, 1, ...
    Point,   // x, y
    Vector,  // x, y
    Rect,    // x, y, z: width, w: height
    Color,   // x, y, z, w: red, green, blue, alpha, 0 to 255
    Entity,  // x: the entity as its 32-bit number (index and version); 4294967295 is none
    Text,    // none: a std::string, read and written with FieldDescriptor::readText and writeText
    PointList, // none: a std::vector<pxPoint>, read and written with readPoints and writePoints
    RecordList, // none: vector<ComponentRecord<T>>; T uses its registered component descriptors
};

// A scalar field's value (excluding Text, PointList and RecordList), in doubles; see FieldKind for which
// members are used.
struct FieldValue {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 0.0;
};

// The editor a tool should use for a field; Auto lets the tool choose from the kind. The values
// mirror PixieInspector's EditorKind, through the C API's PIXIE_EDITOR_* constants.
enum class FieldEditor : std::uint8_t { Auto, Number, Slider, Toggle, Choice, Color, Arc, LayerMask, LayerNames };

// Which way dragging a field's label changes it.
enum class ScrubAxis : std::uint8_t { Horizontal, Vertical };

// How a tool shows and edits a field: the vocabulary of PixieInspector's node options. Range, step,
// and scrub are in the displayed unit, the one a person reads and types.
struct FieldMetadata {
    // Shown instead of the name when not empty.
    std::string label;
    std::string tooltip;
    std::string help;
    // Fields are shown by order; Field() sets it to the declaration position.
    int order = 0;
    bool readOnly = false;
    FieldEditor editor = FieldEditor::Auto;
    bool hasRange = false;
    double rangeMin = 0.0;
    double rangeMax = 0.0;
    // 0 means no step.
    double rangeStep = 0.0;
    // Change per pixel dragged on the label; 0 means no scrubbing.
    double scrub = 0.0;
    ScrubAxis scrubAxis = ScrubAxis::Horizontal;
    // shown = held * unitFactor + unitOffset, followed by unitSuffix.
    bool hasUnit = false;
    std::string unitSuffix;
    double unitFactor = 1.0;
    double unitOffset = 0.0;
    // For an Arc field (the center angle): the name of the sibling field with the arc's width.
    std::string arcSpread;
    // For an Entity field: the component type an entity needs to be a valid choice.
    std::string references;
    // For a Text field that names a file: the file's kind, as its extension without the dot
    // ("png"); the path is relative to the project folder. Empty for plain text.
    std::string asset;
};

// One reflected field: a name, a kind, how to read and write it in a component instance, and how a
// tool should show it. A Text field is read and written as text, a PointList field as points, the
// others as a FieldValue; the other pairs of functions are null.
struct FieldDescriptor {
    std::string name;
    FieldKind kind = FieldKind::Double;
    std::vector<std::string> enumNames;
    FieldValue (*read)(const void* component) = nullptr;
    void (*write)(void* component, const FieldValue& value) = nullptr;
    std::string (*readText)(const void* component) = nullptr;
    void (*writeText)(void* component, std::string_view text) = nullptr;
    std::vector<pxPoint> (*readPoints)(const void* component) = nullptr;
    void (*writePoints)(void* component, std::span<const pxPoint> points) = nullptr;
    FieldMetadata metadata;
    entt::id_type recordType = 0;
    std::size_t (*recordCount)(const void*) = nullptr;
    void* (*recordAt)(void*, std::size_t) = nullptr;
    std::string& (*recordExtras)(void*, std::size_t) = nullptr;
    void (*insertRecord)(void*, std::size_t) = nullptr;
    void (*eraseRecord)(void*, std::size_t) = nullptr;
    // Builds a temporary list, populates it and only then replaces the old value.
    void (*replaceRecords)(void*, std::size_t,
        const std::function<void(void*, std::string&, std::size_t)>&) = nullptr;
    static constexpr std::size_t maxRecords = 256;
};

// Saved fields that a type's current version no longer has, by name, offered to its upgrade: each
// one that is a number (x) or an array of up to four numbers (x, y, z, w). Others are not offered.
using LegacyFields = std::map<std::string, FieldValue, std::less<>>;

// One reflected component type: its name, its fields, and how to find, add, and remove it on an
// entity.
struct ComponentDescriptor {
    std::string name;
    entt::id_type type = 0;
    std::vector<FieldDescriptor> fields;
    // The version of the type's fields, saved with every instance in a scene; raise it when a field
    // changes meaning.
    int version = 1;
    // A scenario holds the values a scene is built from, rather than live state; a tool offers to
    // rebuild the scene after editing one.
    bool scenario = false;
    // A reflected record value, not an independently addable/serialized ECS component.
    bool valueType = false;
    // Non-null marks a single-instance type and checks whether any instance already exists.
    // Tools also preflight batch cardinality before applying individual additions.
    bool (*singleton)(const World& world) = nullptr;
    // The component of `entity`, or null when it has none.
    void* (*find)(entt::registry& registry, entt::entity entity) = nullptr;
    // The component of `entity`, added with its defaults when it has none; a Behaviour is added as
    // Entity::AddBehaviour adds it. Null for a type that is not default-constructible. Not during a
    // step.
    void* (*add)(World& world, entt::entity entity) = nullptr;
    // Removes the component from `entity`, if it has one. Not during a step.
    void (*remove)(World& world, entt::entity entity) = nullptr;
    // Optional authoring admission rule. Scene loading and direct Entity::Add stay permissive,
    // so legacy/invalid scenes can be diagnosed and repaired without silently dropping data.
    std::string (*additionError)(const World& world, entt::entity entity) = nullptr;
    // Optional conversion of an instance saved at an older version, called after its current fields
    // are read. It erases from `legacy` each field it consumed; the rest are kept like unknown fields.
    void (*upgrade)(void* component, int savedVersion, LegacyFields& legacy) = nullptr;

    std::string AdditionError(World& world, entt::entity entity) const {
        if (!world.Registry().valid(entity)) return "The entity no longer exists.";
        if (valueType) return name + " is a record value, not an entity component.";
        if (find(world.Registry(), entity)) return {};
        if (!add) return name + " has no default value to add.";
        if (singleton && singleton(world)) return "This World already has " + name + ". Edit the existing component.";
        return additionError ? additionError(world, entity) : std::string{};
    }
};

template<typename T>
class ComponentBuilder;

// The component types a tool can see and edit: a list of descriptors, filled in code, one builder
// call per field, with the field's metadata chained after it. It is the single source of what a field
// is and how it is shown: the C API exports it, the editor builds its inspectors from it, and
// serialization is meant to read it. It holds no state of a World.
class ComponentTypes {
public:
    template<typename T>
    ComponentBuilder<T> Add(std::string name);

    const std::vector<ComponentDescriptor>& All() const { return types_; }
    // The index of the type named `name` in All(), or -1.
    int IndexOf(std::string_view name) const;

private:
    template<typename T>
    friend class ComponentBuilder;

    std::vector<ComponentDescriptor> types_;
};

// --- Implementation ------------------------------------------------------------------------------

namespace detail {

template<typename M> struct RecordListTraits { static constexpr bool value = false; };
template<typename T> struct RecordListTraits<std::vector<ComponentRecord<T>>> {
    static constexpr bool value = true;
    using Item = T;
};

template<typename T>
struct MemberTraits;

template<typename C, typename M>
struct MemberTraits<M C::*> {
    using Class = C;
    using Type = M;
};

template<typename M>
constexpr FieldKind KindOf() {
    if constexpr (std::is_same_v<M, bool>) {
        return FieldKind::Bool;
    } else if constexpr (std::is_same_v<M, entt::entity>) {
        // Before the enum test: entt::entity is an enum class.
        return FieldKind::Entity;
    } else if constexpr (std::is_enum_v<M>) {
        return FieldKind::Enum;
    } else if constexpr (std::is_integral_v<M>) {
        return FieldKind::Int;
    } else if constexpr (std::is_same_v<M, float>) {
        return FieldKind::Float;
    } else if constexpr (std::is_same_v<M, double>) {
        return FieldKind::Double;
    } else if constexpr (std::is_same_v<M, pxPoint>) {
        return FieldKind::Point;
    } else if constexpr (std::is_same_v<M, pxVec2>) {
        return FieldKind::Vector;
    } else if constexpr (std::is_same_v<M, pxRect>) {
        return FieldKind::Rect;
    } else if constexpr (std::is_same_v<M, pxColorRgba>) {
        return FieldKind::Color;
    } else if constexpr (std::is_same_v<M, std::string>) {
        return FieldKind::Text;
    } else if constexpr (std::is_same_v<M, std::vector<pxPoint>>) {
        return FieldKind::PointList;
    } else if constexpr (RecordListTraits<M>::value) {
        return FieldKind::RecordList;
    } else {
        static_assert(sizeof(M) == 0, "this field type cannot be reflected");
    }
}

inline std::uint8_t ToByte(double v) {
    return static_cast<std::uint8_t>(v < 0.0 ? 0.0 : v > 255.0 ? 255.0 : v + 0.5);
}

template<typename M>
FieldValue ToValue(const M& v) {
    if constexpr (std::is_same_v<M, entt::entity>) {
        return {static_cast<double>(static_cast<std::uint32_t>(v))};
    } else if constexpr (std::is_enum_v<M>) {
        return {static_cast<double>(static_cast<std::underlying_type_t<M>>(v))};
    } else if constexpr (std::is_arithmetic_v<M>) {
        return {static_cast<double>(v)};
    } else if constexpr (std::is_same_v<M, pxPoint> || std::is_same_v<M, pxVec2>) {
        return {v.x, v.y};
    } else if constexpr (std::is_same_v<M, pxRect>) {
        return {v.x, v.y, v.width, v.height};
    } else {
        return {double(v.r), double(v.g), double(v.b), double(v.a)};
    }
}

template<typename M>
void FromValue(M& target, const FieldValue& v) {
    if constexpr (std::is_same_v<M, bool>) {
        target = v.x != 0.0;
    } else if constexpr (std::is_same_v<M, entt::entity>) {
        target = static_cast<entt::entity>(static_cast<std::uint32_t>(v.x));
    } else if constexpr (std::is_enum_v<M>) {
        target = static_cast<M>(static_cast<std::underlying_type_t<M>>(v.x));
    } else if constexpr (std::is_arithmetic_v<M>) {
        target = static_cast<M>(v.x);
    } else if constexpr (std::is_same_v<M, pxPoint>) {
        target = pxPoint(v.x, v.y);
    } else if constexpr (std::is_same_v<M, pxVec2>) {
        target = pxVec2(v.x, v.y);
    } else if constexpr (std::is_same_v<M, pxRect>) {
        target = pxRect(v.x, v.y, v.z, v.w);
    } else {
        target = pxColorRgba(ToByte(v.x), ToByte(v.y), ToByte(v.z), ToByte(v.w));
    }
}

template<auto Member>
FieldValue Read(const void* component) {
    using Class = typename MemberTraits<decltype(Member)>::Class;
    return ToValue(static_cast<const Class*>(component)->*Member);
}

template<auto Member>
void Write(void* component, const FieldValue& value) {
    using Class = typename MemberTraits<decltype(Member)>::Class;
    FromValue(static_cast<Class*>(component)->*Member, value);
}

template<auto Member>
std::string ReadText(const void* component) {
    using Class = typename MemberTraits<decltype(Member)>::Class;
    return static_cast<const Class*>(component)->*Member;
}

template<auto Member>
void WriteText(void* component, std::string_view text) {
    using Class = typename MemberTraits<decltype(Member)>::Class;
    static_cast<Class*>(component)->*Member = std::string(text);
}

template<auto Member>
std::vector<pxPoint> ReadPoints(const void* component) {
    using Class = typename MemberTraits<decltype(Member)>::Class;
    return static_cast<const Class*>(component)->*Member;
}

template<auto Member>
void WritePoints(void* component, std::span<const pxPoint> points) {
    using Class = typename MemberTraits<decltype(Member)>::Class;
    static_cast<Class*>(component)->*Member = std::vector<pxPoint>(points.begin(), points.end());
}

template<typename T>
void* Find(entt::registry& registry, entt::entity entity) {
    return registry.valid(entity) ? registry.try_get<T>(entity) : nullptr;
}

template<typename T>
void* AddTo(World& world, entt::entity entity) {
    if (!world.Registry().valid(entity)) {
        return nullptr;
    }
    const Entity handle = world.Wrap(entity);
    if (T* existing = handle.TryGet<T>()) {
        return existing;
    }
    if constexpr (std::derived_from<T, Behaviour>) {
        return &handle.AddBehaviour<T>();
    } else {
        return &handle.Add<T>();
    }
}

template<typename T>
void RemoveFrom(World& world, entt::entity entity) {
    if (world.Registry().valid(entity)) {
        world.Wrap(entity).Remove<T>();
    }
}

} // namespace detail

// Adds the fields of one component type to its ComponentTypes.
template<typename T>
class ComponentBuilder {
public:
    ComponentBuilder(ComponentTypes& types, std::size_t index) : types_(types), index_(index) {}

    // Field<&T::member>("name"); enumNames name an enum's values 0, 1, ...
    template<auto Member>
    ComponentBuilder& Field(std::string name, std::vector<std::string> enumNames = {}) {
        using Traits = detail::MemberTraits<decltype(Member)>;
        static_assert(std::is_base_of_v<typename Traits::Class, T>, "the member must belong to T");
        FieldDescriptor field;
        field.name = std::move(name);
        field.kind = detail::KindOf<typename Traits::Type>();
        field.enumNames = std::move(enumNames);
        if constexpr (std::is_same_v<typename Traits::Type, std::string>) {
            field.readText = &detail::ReadText<Member>;
            field.writeText = &detail::WriteText<Member>;
        } else if constexpr (std::is_same_v<typename Traits::Type, std::vector<pxPoint>>) {
            field.readPoints = &detail::ReadPoints<Member>;
            field.writePoints = &detail::WritePoints<Member>;
        } else if constexpr (detail::RecordListTraits<typename Traits::Type>::value) {
            using List = typename Traits::Type;
            using Item = typename detail::RecordListTraits<List>::Item;
            field.recordType = entt::type_hash<Item>::value();
            field.recordCount = [](const void* c) { return (static_cast<const T*>(c)->*Member).size(); };
            field.recordAt = [](void* c, std::size_t i) -> void* { return &(static_cast<T*>(c)->*Member).at(i).value; };
            field.recordExtras = [](void* c, std::size_t i) -> std::string& { return (static_cast<T*>(c)->*Member).at(i).unknownFields; };
            field.insertRecord = [](void* c, std::size_t i) {
                auto& list = static_cast<T*>(c)->*Member;
                if (list.size() >= FieldDescriptor::maxRecords || i > list.size()) throw std::out_of_range("record list insertion");
                list.insert(list.begin() + static_cast<std::ptrdiff_t>(i), ComponentRecord<Item>{});
            };
            field.eraseRecord = [](void* c, std::size_t i) {
                auto& list = static_cast<T*>(c)->*Member;
                if (i >= list.size()) throw std::out_of_range("record list removal");
                list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
            };
            field.replaceRecords = [](void* c, std::size_t count,
                const std::function<void(void*, std::string&, std::size_t)>& populate) {
                if (count > FieldDescriptor::maxRecords) throw std::out_of_range("record list exceeds 256 items");
                List next(count);
                for (std::size_t i = 0; i < count; ++i) populate(&next[i].value, next[i].unknownFields, i);
                static_cast<T*>(c)->*Member = std::move(next);
            };
        } else {
            field.read = &detail::Read<Member>;
            field.write = &detail::Write<Member>;
        }
        auto& fields = types_.types_[index_].fields;
        field.metadata.order = static_cast<int>(fields.size());
        fields.push_back(std::move(field));
        return *this;
    }

    // Marks the type as a scenario: values the scene is built from (see ComponentDescriptor).
    ComponentBuilder& Scenario() {
        types_.types_[index_].scenario = true;
        return *this;
    }

    ComponentBuilder& ValueType() {
        types_.types_[index_].valueType = true;
        types_.types_[index_].add = nullptr;
        return *this;
    }

    // A native, read-only reason why a tool cannot add this component; empty means allowed.
    ComponentBuilder& CanAdd(std::string (*check)(const World&, entt::entity)) {
        types_.types_[index_].additionError = check;
        return *this;
    }

    ComponentBuilder& Singleton() {
        types_.types_[index_].singleton = [](const World& world) {
            const auto view = world.Registry().view<const T>();
            return view.begin() != view.end();
        };
        return *this;
    }

    // The version of the type's fields (see ComponentDescriptor); 1 when not given.
    ComponentBuilder& Version(int version) {
        types_.types_[index_].version = version;
        return *this;
    }

    // Upgrade<&Convert>() converts instances saved at an older version (see
    // ComponentDescriptor::upgrade).
    template<void (*Convert)(T& component, int savedVersion, LegacyFields& legacy)>
    ComponentBuilder& Upgrade() {
        types_.types_[index_].upgrade = [](void* component, int savedVersion, LegacyFields& legacy) {
            Convert(*static_cast<T*>(component), savedVersion, legacy);
        };
        return *this;
    }

    // Metadata of the field declared last.
    ComponentBuilder& Label(std::string text) {
        return Last([&](FieldMetadata& m) { m.label = std::move(text); });
    }
    ComponentBuilder& Tooltip(std::string text) {
        return Last([&](FieldMetadata& m) { m.tooltip = std::move(text); });
    }
    ComponentBuilder& Help(std::string text) {
        return Last([&](FieldMetadata& m) { m.help = std::move(text); });
    }
    ComponentBuilder& Order(int order) {
        return Last([&](FieldMetadata& m) { m.order = order; });
    }
    ComponentBuilder& ReadOnly() {
        return Last([](FieldMetadata& m) { m.readOnly = true; });
    }
    ComponentBuilder& Editor(FieldEditor editor) {
        return Last([&](FieldMetadata& m) { m.editor = editor; });
    }

    // Allowed values and the step between them, in the displayed unit.
    ComponentBuilder& Range(double min, double max, double step = 0.0) {
        return Last([&](FieldMetadata& m) {
            m.hasRange = true;
            m.rangeMin = min;
            m.rangeMax = max;
            m.rangeStep = step;
        });
    }

    // Change per pixel dragged on the label, in the displayed unit.
    ComponentBuilder& Scrub(double perPixel, ScrubAxis axis = ScrubAxis::Horizontal) {
        return Last([&](FieldMetadata& m) {
            m.scrub = perPixel;
            m.scrubAxis = axis;
        });
    }

    // Shown as held * factor + offset, followed by `suffix`.
    ComponentBuilder& Unit(std::string suffix, double factor = 1.0, double offset = 0.0) {
        return Last([&](FieldMetadata& m) {
            m.hasUnit = true;
            m.unitSuffix = std::move(suffix);
            m.unitFactor = factor;
            m.unitOffset = offset;
        });
    }

    // An angle held in radians, shown in degrees, with `suffix` after it ("°", "°/s").
    ComponentBuilder& Degrees(std::string suffix = "°") {
        return Unit(std::move(suffix), kRadiansToDegrees);
    }

    // An Entity field: the entities that have component `type` are the valid choices.
    ComponentBuilder& References(std::string type) {
        return Last([&](FieldMetadata& m) { m.references = std::move(type); });
    }

    // A Text field naming a file of kind `extension` ("png"), relative to the project folder.
    ComponentBuilder& Asset(std::string extension) {
        return Last([&](FieldMetadata& m) { m.asset = std::move(extension); });
    }

    // This angle is the center of an arc whose width is the sibling field `spread`.
    ComponentBuilder& Arc(std::string spread) {
        return Last([&](FieldMetadata& m) {
            m.editor = FieldEditor::Arc;
            m.arcSpread = std::move(spread);
        });
    }

    static constexpr double kRadiansToDegrees = 57.29577951308232;

private:
    template<typename Change>
    ComponentBuilder& Last(Change&& change) {
        auto& fields = types_.types_[index_].fields;
        if (!fields.empty()) {
            change(fields.back().metadata);
        }
        return *this;
    }

    ComponentTypes& types_;
    std::size_t index_;
};

template<typename T>
ComponentBuilder<T> ComponentTypes::Add(std::string name) {
    static_assert(!std::is_empty_v<T>, "an empty (tag) type has no instances to reflect");
    ComponentDescriptor descriptor;
    descriptor.name = std::move(name);
    descriptor.type = entt::type_hash<T>::value();
    descriptor.find = &detail::Find<T>;
    if constexpr (std::is_default_constructible_v<T>) {
        descriptor.add = &detail::AddTo<T>;
    }
    descriptor.remove = &detail::RemoveFrom<T>;
    types_.push_back(std::move(descriptor));
    return ComponentBuilder<T>(*this, types_.size() - 1);
}

inline int ComponentTypes::IndexOf(std::string_view name) const {
    for (std::size_t i = 0; i < types_.size(); ++i) {
        if (types_[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Describes the runtime's own components, with their metadata: Transform2D, Velocity2D,
// Acceleration2D, Shape, Sprite, Text, Path, Attractor, ParticleSystem, and ParticleSpawner.
void DescribeRuntimeComponents(ComponentTypes& types);

} // namespace pixie
