#pragma once

#include "pixie/runtime/Export.hpp"

#include <entt/entity/entity.hpp>

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace pixie {

// A uniform grid over the plane that answers "what is within this radius of that point". Cells
// are squares of CellSize(); occupied cells are found through a hash table sized to the number of
// items, so the grid has no bounds and costs nothing for empty space.
//
// Fill it with Clear, Add for each item, then Build; query it until the next Clear. It holds a
// snapshot: moving an entity afterwards does not move it in the grid.
class SpatialGrid {
public:
    explicit SpatialGrid(double cellSize = 64.0) { SetCellSize(cellSize); }

    // A cell size near the usual query radius keeps queries cheap. Takes effect at the next Build.
    PIXIE_RUNTIME_API void SetCellSize(double cellSize);
    double CellSize() const { return cellSize_; }

    PIXIE_RUNTIME_API void Clear();
    PIXIE_RUNTIME_API void Add(entt::entity id, pxPoint position);
    PIXIE_RUNTIME_API void Build();

    std::size_t Size() const { return items_.size(); }

    // Calls fn(id, position, distanceSquared) once for every item within `radius` of `center`,
    // the boundary included.
    template<typename Fn>
    void ForEachWithin(pxPoint center, double radius, Fn&& fn) const;

private:
    struct Item {
        pxPoint position;
        entt::entity id;
        std::int32_t cellX;
        std::int32_t cellY;
    };

    std::int32_t CellOf(double coordinate) const {
        return static_cast<std::int32_t>(std::floor(coordinate * inverseCellSize_));
    }
    PIXIE_RUNTIME_API std::size_t Bucket(std::int32_t cellX, std::int32_t cellY) const;

    double cellSize_ = 64.0;
    double inverseCellSize_ = 1.0 / 64.0;
    std::vector<Item> items_;
    // After Build: items_ sorted by bucket, and bucket b holding items_[starts_[b], starts_[b + 1]).
    std::vector<std::uint32_t> starts_;
    std::size_t bucketMask_ = 0;
};

template<typename Fn>
void SpatialGrid::ForEachWithin(pxPoint center, double radius, Fn&& fn) const {
    if (items_.empty() || starts_.empty()) {
        return;
    }
    const double radiusSquared = radius * radius;
    const std::int32_t x0 = CellOf(center.x - radius);
    const std::int32_t x1 = CellOf(center.x + radius);
    const std::int32_t y0 = CellOf(center.y - radius);
    const std::int32_t y1 = CellOf(center.y + radius);
    for (std::int32_t cy = y0; cy <= y1; ++cy) {
        for (std::int32_t cx = x0; cx <= x1; ++cx) {
            const std::size_t bucket = Bucket(cx, cy);
            for (std::uint32_t i = starts_[bucket]; i < starts_[bucket + 1]; ++i) {
                const Item& item = items_[i];
                // Several cells share a bucket; count each item only in its own cell.
                if (item.cellX != cx || item.cellY != cy) {
                    continue;
                }
                const pxVec2 offset = item.position - center;
                const double distanceSquared = glm::dot(offset, offset);
                if (distanceSquared <= radiusSquared) {
                    fn(item.id, item.position, distanceSquared);
                }
            }
        }
    }
}

} // namespace pixie
