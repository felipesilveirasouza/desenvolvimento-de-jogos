#include "Collision2D.hpp"

// --- AABB ------------------------------------------------------------------

bool AABB::intersects(const AABB& other) const noexcept {
    // Two boxes overlap only if their intervals overlap on BOTH axes.
    // Using <= / >= makes touching edges count as overlapping.
    return min.x <= other.max.x && max.x >= other.min.x
        && min.y <= other.max.y && max.y >= other.min.y;
}

// --- Collision2D -----------------------------------------------------------

AABB Collision2D::bounds(const Vector2D& position) const noexcept {
    return AABB{ position - halfExtents, position + halfExtents };
}
