#include "RigidBody2D.hpp"

#include <cassert>

void RigidBody2D::integrate(float dt) noexcept {
    assert(dt > 0.0f && "timestep must be positive");

    // Semi-implicit Euler: update velocity FIRST, then use the NEW velocity
    // to update the position. Swapping these two lines gives explicit Euler,
    // which gains energy over time and becomes unstable.
    velocity += acceleration * dt;
    position += velocity * dt;
}
