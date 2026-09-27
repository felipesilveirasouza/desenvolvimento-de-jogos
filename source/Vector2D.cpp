#include "Vector2D.hpp"

#include <cassert>
#include <cmath>

// --- Length ----------------------------------------------------------------

Vector2D Vector2D::normalized() const {
    const float len = length();
    assert(len > EPSILON && "cannot normalize a (near) zero vector");
    return Vector2D(x / len, y / len);
}

void Vector2D::normalize() {
    *this = normalized();
}

bool Vector2D::equals(const Vector2D& rhs, float tolerance) const noexcept {
    return std::abs(x - rhs.x) <= tolerance
        && std::abs(y - rhs.y) <= tolerance;
}

// --- Operators -------------------------------------------------------------

Vector2D Vector2D::operator+(const Vector2D& rhs) const noexcept {
    return Vector2D(x + rhs.x, y + rhs.y);
}

Vector2D Vector2D::operator-(const Vector2D& rhs) const noexcept {
    return Vector2D(x - rhs.x, y - rhs.y);
}

Vector2D Vector2D::operator*(float scalar) const noexcept {
    return Vector2D(x * scalar, y * scalar);
}

Vector2D Vector2D::operator/(float scalar) const {
    assert(std::abs(scalar) > EPSILON && "division by (near) zero");
    return Vector2D(x / scalar, y / scalar);
}

Vector2D& Vector2D::operator+=(const Vector2D& rhs) noexcept {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

Vector2D& Vector2D::operator-=(const Vector2D& rhs) noexcept {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

Vector2D& Vector2D::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2D& Vector2D::operator/=(float scalar) {
    assert(std::abs(scalar) > EPSILON && "division by (near) zero");
    x /= scalar;
    y /= scalar;
    return *this;
}

Vector2D operator*(float scalar, const Vector2D& vec) noexcept {
    return vec * scalar;
}
