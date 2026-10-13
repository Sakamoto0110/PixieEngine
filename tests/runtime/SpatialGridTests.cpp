#include "Check.hpp"

#include "pixie/runtime/Random.hpp"
#include "pixie/runtime/SpatialGrid.hpp"

#include <algorithm>
#include <vector>

using namespace pixie;

namespace {

struct Point {
    entt::entity id;
    pxPoint position;
};

std::vector<Point> RandomPoints(Random& random, int count, double half) {
    std::vector<Point> points;
    for (int i = 0; i < count; ++i) {
        points.push_back({static_cast<entt::entity>(i),
                          pxPoint(random.Range(-half, half), random.Range(-half, half))});
    }
    return points;
}

std::vector<entt::entity> BruteForce(const std::vector<Point>& points, pxPoint center, double radius) {
    std::vector<entt::entity> found;
    for (const Point& p : points) {
        const pxVec2 d = p.position - center;
        if (glm::dot(d, d) <= radius * radius) {
            found.push_back(p.id);
        }
    }
    std::sort(found.begin(), found.end());
    return found;
}

std::vector<entt::entity> Query(const SpatialGrid& grid, pxPoint center, double radius) {
    std::vector<entt::entity> found;
    grid.ForEachWithin(center, radius, [&](entt::entity id, pxPoint, double) { found.push_back(id); });
    std::sort(found.begin(), found.end());
    return found;
}

} // namespace

TEST(GridAgreesWithBruteForce) {
    Random random(11);
    const std::vector<Point> points = RandomPoints(random, 3000, 600.0);
    for (const double cellSize : {7.0, 50.0, 333.0}) {
        SpatialGrid grid(cellSize);
        for (const Point& p : points) {
            grid.Add(p.id, p.position);
        }
        grid.Build();
        CHECK(grid.Size() == points.size());
        for (int q = 0; q < 200; ++q) {
            const pxPoint center(random.Range(-700.0, 700.0), random.Range(-700.0, 700.0));
            const double radius = random.Range(0.0, 260.0);
            const auto found = Query(grid, center, radius);
            CHECK(found == BruteForce(points, center, radius));
            // Each item at most once: the sorted result has no duplicates.
            CHECK(std::adjacent_find(found.begin(), found.end()) == found.end());
        }
    }
}

TEST(GridReportsDistanceAndIncludesTheBoundary) {
    SpatialGrid grid(10.0);
    grid.Add(entt::entity{1}, pxPoint(3.0, 4.0));
    grid.Build();
    int calls = 0;
    grid.ForEachWithin(pxPoint(0.0, 0.0), 5.0, [&](entt::entity id, pxPoint position, double d2) {
        ++calls;
        CHECK(id == entt::entity{1});
        CHECK(position == pxPoint(3.0, 4.0));
        CHECK(d2 == 25.0);
    });
    CHECK(calls == 1);
}

TEST(EmptyOrClearedGridFindsNothing) {
    SpatialGrid grid;
    int calls = 0;
    grid.ForEachWithin(pxPoint(0.0, 0.0), 1e6, [&](entt::entity, pxPoint, double) { ++calls; });
    grid.Add(entt::entity{1}, pxPoint(0.0, 0.0));
    grid.Build();
    grid.Clear();
    grid.ForEachWithin(pxPoint(0.0, 0.0), 1e6, [&](entt::entity, pxPoint, double) { ++calls; });
    CHECK(calls == 0);
}
