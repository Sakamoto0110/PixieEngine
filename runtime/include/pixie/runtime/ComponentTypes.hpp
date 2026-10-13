#pragma once

#include <entt/entity/registry.hpp>

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pixie {

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
};

// A field's value, whatever its kind, in doubles; see FieldKind for which members are used.
struct FieldValue {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 0.0;
};

// One reflected field: a name, a kind, and how to read and write it in a component instance.
struct FieldDescriptor {
    std::string name;
    FieldKind kind = FieldKind::Double;
    std::vector<std::string> enumNames;
    FieldValue (*read)(const void* component) = nullptr;
    void (*write)(void* component, const FieldValue& value) = nullptr;
};

// One reflected component type: its name, its fields, and how to find it on an entity.
struct ComponentDescriptor {
    std::string name;
    entt::id_type type = 0;
    std::vector<FieldDescriptor> fields;
    // The component of `entity`, or null when it has none.
    void* (*find)(entt::registry& registry, entt::entity entity) = nullptr;
};

template<typename T>
class ComponentBuilder;

// The component types a tool can see and edit: a list of descriptors, filled in code, one builder
// call per field. The C API, the editor's proxies, and later serialization all read fields through
// it, so a component is described once. It holds no state of a World.
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
    } else {
        static_assert(sizeof(M) == 0, "this field type cannot be reflected");
    }
}

inline std::uint8_t ToByte(double v) {
    return static_cast<std::uint8_t>(v < 0.0 ? 0.0 : v > 255.0 ? 255.0 : v + 0.5);
}

template<typename M>
FieldValue ToValue(const M& v) {
    if constexpr (std::is_enum_v<M>) {
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

template<typename T>
void* Find(entt::registry& registry, entt::entity entity) {
    return registry.valid(entity) ? registry.try_get<T>(entity) : nullptr;
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
        field.read = &detail::Read<Member>;
        field.write = &detail::Write<Member>;
        types_.types_[index_].fields.push_back(std::move(field));
        return *this;
    }

private:
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

// Describes the runtime's own components: Transform2D, Velocity2D, Acceleration2D, Shape,
// Attractor, and ParticleEmitter.
void DescribeRuntimeComponents(ComponentTypes& types);

} // namespace pixie
