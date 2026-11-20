#include "pixie/runtime/Parenting2D.hpp"

#include "Parenting2DInternal.hpp"
#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace pixie {
namespace {

struct ParentLink {
    entt::entity parent;
    Transform2D local;
    Transform2D published;
    Transform2D previousLocal;
    bool snap = true;
};

using Links = std::map<entt::entity, ParentLink>;
using Poses = std::map<entt::entity, Transform2D>;

Links* FindLinks(World& world) {
    return world.Registry().ctx().find<Links>();
}

bool Finite(const Transform2D& pose) {
    return std::isfinite(pose.position.x) && std::isfinite(pose.position.y) &&
        std::isfinite(pose.rotation) && std::isfinite(pose.scale.x) && std::isfinite(pose.scale.y);
}

bool ValidParentPose(const Transform2D& pose) {
    return Finite(pose) && pose.scale.x == pose.scale.y && pose.scale.x > 0 &&
        std::isfinite(1.0 / pose.scale.x);
}

pxVec2 Rotate(pxVec2 value, double angle) {
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return {value.x * cosine - value.y * sine, value.x * sine + value.y * cosine};
}

Transform2D Compose(const Transform2D& parent, const Transform2D& local) {
    const auto offset = Rotate({local.position.x, local.position.y}, parent.rotation) * parent.scale.x;
    return {parent.position + offset, parent.rotation + local.rotation, local.scale * parent.scale.x};
}

Transform2D RelativeTo(const Transform2D& parent, const Transform2D& world) {
    const auto offset = Rotate(world.position - parent.position, -parent.rotation) * (1.0 / parent.scale.x);
    return {{offset.x, offset.y}, world.rotation - parent.rotation, world.scale * (1.0 / parent.scale.x)};
}

bool SamePose(const Transform2D& first, const Transform2D& second) {
    return first.position == second.position && first.rotation == second.rotation &&
        first.scale.x == second.scale.x && first.scale.y == second.scale.y;
}

const Transform2D& Resolved(const World& world, const Poses& poses, entt::entity id) {
    const auto pose = poses.find(id);
    if (pose != poses.end()) return pose->second;
    return world.Registry().get<const Transform2D>(id);
}

std::string Evaluate(World& world, const Links& links, Poses& poses, const Poses& rootEdits = {}) {
    const auto& registry = world.Registry();
    for (const auto& [id, link] : links) {
        if (!registry.valid(id) || !registry.valid(link.parent) ||
            !registry.all_of<Transform2D>(id) || !registry.all_of<Transform2D>(link.parent)) {
            return "parenting requires live entities with Transform2D";
        }
        if (!SamePose(registry.get<const Transform2D>(id), link.published)) {
            return "an attached Transform2D was written directly; use SetLocalTransform2D";
        }
        if (registry.any_of<Velocity2D, Acceleration2D, RigidBody2D, CharacterMotor2D,
                            Collider2D, CompoundCollider2D>(id)) {
            return "a parented child cannot own an independent movement or collider component";
        }
        if (registry.all_of<SceneId>(id) && (!registry.all_of<SceneId>(link.parent) ||
            SceneFormatVersion(world) == 2 || world.ScopeOf(world.Wrap(id)) != ScopeId{})) {
            return "an authored child needs an authored parent in the parenting scene family";
        }
        if (world.ScopeOf(world.Wrap(id)) != world.ScopeOf(world.Wrap(link.parent))) {
            return "parent and child must belong to the same scope";
        }
    }

    std::vector<entt::entity> path;
    for (const auto& entry : links) {
        auto id = entry.first;
        path.clear();
        while (!poses.contains(id)) {
            const auto link = links.find(id);
            if (link == links.end()) break;
            if (path.size() == links.size()) return "parenting would create a cycle";
            path.push_back(id);
            id = link->second.parent;
        }
        const auto edit = rootEdits.find(id);
        auto parentPose = edit != rootEdits.end() ? edit->second : Resolved(world, poses, id);
        for (auto child = path.rbegin(); child != path.rend(); ++child) {
            if (!ValidParentPose(parentPose)) return "a parent needs a finite pose and positive uniform scale";
            auto pose = Compose(parentPose, links.at(*child).local);
            if (!Finite(pose)) return "the evaluated child pose is outside the supported range";
            poses.emplace(*child, pose);
            parentPose = pose;
        }
    }
    return {};
}

void Publish(World& world, Links& links, const Poses& poses) {
    for (const auto& [id, pose] : poses) {
        world.Registry().get<Transform2D>(id) = pose;
        links.at(id).published = pose;
    }
}

Transform2D InterpolateWorldPose(const entt::registry& registry, entt::entity id, double alpha) {
    const auto& current = registry.get<const Transform2D>(id);
    const auto* previous = registry.try_get<const PreviousTransform2D>(id);
    return previous ? Interpolate(previous->value, current, alpha) : current;
}

std::string Commit(World& world, Links next, const Poses& rootEdits = {}) {
    // Resolve the entire proposed graph before publishing either links or poses.
    Poses poses;
    if (auto error = Evaluate(world, next, poses, rootEdits); !error.empty()) return error;
    auto& context = world.Registry().ctx();
    auto* links = FindLinks(world);
    if (!links && !next.empty()) links = &context.emplace<Links>();
    if (links) {
        links->swap(next);
        Publish(world, *links, poses);
    }
    for (const auto& [id, pose] : rootEdits) world.Registry().get<Transform2D>(id) = pose;
    return {};
}

} // namespace

Entity Parent2D(Entity child) {
    if (!child.IsAlive()) return {};
    auto& world = child.GetWorld();
    if (const auto* links = FindLinks(world)) {
        const auto found = links->find(child.Id());
        if (found != links->end() && world.Registry().valid(found->second.parent)) {
            return world.Wrap(found->second.parent);
        }
    }
    return {};
}

std::vector<Entity> Children2D(Entity parent) {
    std::vector<Entity> children;
    if (!parent.IsAlive()) return children;
    auto& world = parent.GetWorld();
    if (const auto* links = FindLinks(world)) {
        for (const auto& [id, link] : *links) {
            if (link.parent == parent.Id() && world.Registry().valid(id)) children.push_back(world.Wrap(id));
        }
    }
    return children;
}

std::optional<Transform2D> LocalTransform2D(Entity entity) {
    if (!entity.IsAlive() || !entity.Has<Transform2D>()) return std::nullopt;
    if (const auto* links = FindLinks(entity.GetWorld())) {
        if (const auto found = links->find(entity.Id()); found != links->end()) return found->second.local;
    }
    return entity.Get<Transform2D>();
}

Transform2D InterpolatedTransform2D(Entity entity, double alpha) {
    auto& world = entity.GetWorld();
    const auto& registry = world.Registry();
    const auto* links = FindLinks(world);
    if (!links) return InterpolateWorldPose(registry, entity.Id(), alpha);
    auto link = links->find(entity.Id());
    if (link == links->end()) return InterpolateWorldPose(registry, entity.Id(), alpha);
    if (link->second.snap) return entity.Get<Transform2D>();

    auto pose = Interpolate(link->second.previousLocal, link->second.local, alpha);
    for (;;) {
        const auto parent = link->second.parent;
        link = links->find(parent);
        if (link == links->end()) return Compose(InterpolateWorldPose(registry, parent, alpha), pose);
        pose = Compose(Interpolate(link->second.previousLocal, link->second.local, alpha), pose);
    }
}

std::string SetLocalTransform2D(Entity entity, const Transform2D& pose) {
    if (!entity.IsAlive() || !entity.Has<Transform2D>()) return "entity needs a live Transform2D";
    auto& world = entity.GetWorld();
    if (world.IsStepping()) return "edit the hierarchy between steps or through World::Defer";
    if (!Finite(pose)) return "the pose must be finite";
    const auto* current = FindLinks(world);
    Links next = current ? *current : Links{};
    if (const auto link = next.find(entity.Id()); link != next.end()) {
        link->second.local = pose;
        return Commit(world, std::move(next));
    }
    return Commit(world, std::move(next), {{entity.Id(), pose}});
}

std::string SetParent2D(Entity child, Entity parent, ReparentMode2D mode) {
    return SetParents2D(std::span(&child, 1), parent, mode);
}

std::string SetParents2D(std::span<const Entity> selection, Entity parent, ReparentMode2D mode, bool* changed) {
    if (changed) *changed = false;
    if (selection.empty() || !selection.front().IsAlive()) return "select at least one live child";
    auto& world = selection.front().GetWorld();
    if (world.IsStepping()) return "edit the hierarchy between steps or through World::Defer";
    const bool detach = parent == Entity{};
    if (!detach && (!parent.IsAlive() || &parent.GetWorld() != &world || !parent.Has<Transform2D>())) {
        return "parent needs a live Transform2D in the same World";
    }
    if (mode != ReparentMode2D::KeepWorld && mode != ReparentMode2D::KeepLocal) {
        return "unsupported reparent mode";
    }
    std::set<entt::entity> selected;
    for (const auto child : selection) {
        if (!child.IsAlive() || &child.GetWorld() != &world || !child.Has<Transform2D>()) {
            return "children need live Transform2D components in the same World";
        }
        if (child == parent) return "an entity cannot parent itself";
        selected.insert(child.Id());
    }

    const auto* current = FindLinks(world);
    Links next = current ? *current : Links{};
    Poses poses;
    if (auto error = Evaluate(world, next, poses); !error.empty()) return error;
    const auto* parentPose = detach ? nullptr : &Resolved(world, poses, parent.Id());
    Poses rootEdits;
    std::vector<Entity> moved;
    for (const auto id : selected) {
        const auto child = world.Wrap(id);
        auto ancestor = Parent2D(child);
        if (ancestor == parent) continue;
        while (ancestor && !selected.contains(ancestor.Id())) ancestor = Parent2D(ancestor);
        if (ancestor) continue;

        const auto& childPose = Resolved(world, poses, id);
        auto local = *LocalTransform2D(child);
        if (detach) {
            next.erase(id);
            rootEdits.emplace(id, mode == ReparentMode2D::KeepWorld ? childPose : local);
        } else {
            if (!ValidParentPose(*parentPose)) return "a parent needs a finite pose and positive uniform scale";
            if (mode == ReparentMode2D::KeepWorld) local = RelativeTo(*parentPose, childPose);
            next.insert_or_assign(id, ParentLink{parent.Id(), local, child.Get<Transform2D>(), local});
        }
        moved.push_back(child);
    }
    if (moved.empty()) return {};
    auto error = Commit(world, std::move(next), rootEdits);
    if (error.empty()) {
        for (const auto child : moved) world.ResetInterpolation(child);
        if (changed) *changed = true;
    }
    return error;
}

std::string SetWorldTransform2D(Entity entity, const Transform2D& pose) {
    const auto parent = Parent2D(entity);
    if (!parent) return SetLocalTransform2D(entity, pose);
    auto& world = entity.GetWorld();
    Poses current;
    if (const auto error = Evaluate(world, *FindLinks(world), current); !error.empty()) return error;
    const auto& parentPose = Resolved(world, current, parent.Id());
    return SetLocalTransform2D(entity, RelativeTo(parentPose, pose));
}

namespace detail {

std::string RestoreParenting2D(World& world, std::span<const AuthoredParentLink2D> authored) {
    if (authored.empty()) return {};
    const auto* current = FindLinks(world);
    Links next = current ? *current : Links{};
    for (const auto& [child, parent] : authored) {
        const auto* pose = child.TryGet<Transform2D>();
        if (!pose) return "an authored child needs Transform2D";
        if (!next.try_emplace(child.Id(), ParentLink{parent.Id(), *pose, *pose, *pose}).second) {
            return "an imported child already has a parent";
        }
    }
    auto result = Commit(world, std::move(next));
    if (result.empty()) {
        for (const auto& [child, parent] : authored) {
            (void)parent;
            auto& registry = world.Registry();
            registry.get<PreviousTransform2D>(child.Id()).value = child.Get<Transform2D>();
        }
    }
    return result;
}

void RefreshParenting2D(World& world) {
    auto* links = FindLinks(world);
    if (!links || links->empty()) return;
    Poses poses;
    if (auto error = Evaluate(world, *links, poses); !error.empty()) throw std::logic_error(error);
    Publish(world, *links, poses);
}

void CaptureParenting2D(World& world) {
    if (auto* links = FindLinks(world)) {
        for (auto& entry : *links) {
            entry.second.previousLocal = entry.second.local;
            entry.second.snap = false;
        }
    }
}

void RemoveParenting2D(Entity entity) {
    auto& world = entity.GetWorld();
    auto* links = FindLinks(world);
    if (!links || links->empty()) return;
    const auto children = Children2D(entity);
    if (!children.empty()) RefreshParenting2D(world);
    links->erase(entity.Id());
    for (auto child : children) {
        links->erase(child.Id());
        world.ResetInterpolation(child);
    }
}

void ResetBranchInterpolation2D(Entity root) {
    auto& world = root.GetWorld();
    auto* links = FindLinks(world);
    if (!links || links->empty()) return;
    for (auto& entry : *links) {
        auto next = entry.first;
        for (;;) {
            if (next == root.Id()) {
                auto& registry = world.Registry();
                registry.emplace_or_replace<PreviousTransform2D>(entry.first, registry.get<Transform2D>(entry.first));
                entry.second.previousLocal = entry.second.local;
                entry.second.snap = true;
                break;
            }
            const auto found = links->find(next);
            if (found == links->end()) break;
            next = found->second.parent;
        }
    }
}

} // namespace detail
} // namespace pixie
