#include "pixie/runtime/SpatialGrid.hpp"

#include <algorithm>
#include <bit>
#include <cassert>

namespace pixie {

void SpatialGrid::SetCellSize(double cellSize) {
    assert(cellSize > 0.0);
    cellSize_ = cellSize;
    inverseCellSize_ = 1.0 / cellSize;
}

void SpatialGrid::Clear() {
    items_.clear();
    starts_.clear();
}

void SpatialGrid::Add(entt::entity id, pxPoint position) {
    items_.push_back({position, id, CellOf(position.x), CellOf(position.y)});
}

std::size_t SpatialGrid::Bucket(std::int32_t cellX, std::int32_t cellY) const {
    const auto x = static_cast<std::uint32_t>(cellX);
    const auto y = static_cast<std::uint32_t>(cellY);
    return static_cast<std::size_t>((x * 73856093u) ^ (y * 19349663u)) & bucketMask_;
}

void SpatialGrid::Build() {
    // The cell coordinates were computed with the cell size at Add time; recompute them in case it
    // changed since.
    for (Item& item : items_) {
        item.cellX = CellOf(item.position.x);
        item.cellY = CellOf(item.position.y);
    }
    // About two buckets per item keeps collisions rare.
    const std::size_t buckets = std::bit_ceil(std::max<std::size_t>(64, items_.size() * 2));
    bucketMask_ = buckets - 1;

    // Counting sort of the items by bucket.
    starts_.assign(buckets + 1, 0);
    for (const Item& item : items_) {
        ++starts_[Bucket(item.cellX, item.cellY) + 1];
    }
    for (std::size_t b = 0; b < buckets; ++b) {
        starts_[b + 1] += starts_[b];
    }
    std::vector<Item> sorted(items_.size());
    std::vector<std::uint32_t> next(starts_.begin(), starts_.end() - 1);
    for (const Item& item : items_) {
        sorted[next[Bucket(item.cellX, item.cellY)]++] = item;
    }
    items_ = std::move(sorted);
}

} // namespace pixie
