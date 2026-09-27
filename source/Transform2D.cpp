#include "Transform2D.hpp"

#include <cmath>

// --- Factories -------------------------------------------------------------

Transform2D Transform2D::translation(float tx, float ty) noexcept {
    Transform2D t;          // starts as identity
    t.m[2][0] = tx;         // translation lives in the third row
    t.m[2][1] = ty;
    return t;
}

Transform2D Transform2D::rotation(float angle_rad) noexcept {
    const float c = std::cos(angle_rad);
    const float s = std::sin(angle_rad);

    Transform2D t;
    t.m[0][0] =  c;  t.m[0][1] = s;
    t.m[1][0] = -s;  t.m[1][1] = c;
    return t;
}

Transform2D Transform2D::scale(float sx, float sy) noexcept {
    Transform2D t;
    t.m[0][0] = sx;
    t.m[1][1] = sy;
    return t;
}

// --- Composition -----------------------------------------------------------

Transform2D Transform2D::operator*(const Transform2D& rhs) const noexcept {
    Transform2D result;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            // Assign (not +=): result starts as identity, not zero.
            result.m[row][col] = m[row][0] * rhs.m[0][col]
                               + m[row][1] * rhs.m[1][col]
                               + m[row][2] * rhs.m[2][col];
        }
    }
    return result;
}

Transform2D& Transform2D::operator*=(const Transform2D& rhs) noexcept {
    // Build the product in a temporary first, so reading and writing the
    // same matrix (e.g. t *= t) is safe.
    *this = *this * rhs;
    return *this;
}

// --- Application -----------------------------------------------------------

Vector2D Transform2D::transform_point(const Vector2D& point) const noexcept {
    // [x, y, 1] * M
    return Vector2D(
        point.x * m[0][0] + point.y * m[1][0] + m[2][0],
        point.x * m[0][1] + point.y * m[1][1] + m[2][1]
    );
}

Vector2D Transform2D::transform_vector(const Vector2D& direction) const noexcept {
    // [x, y, 0] * M: the third row (translation) is multiplied by 0.
    return Vector2D(
        direction.x * m[0][0] + direction.y * m[1][0],
        direction.x * m[0][1] + direction.y * m[1][1]
    );
}
