#ifndef VECTOR2D_H
#define VECTOR2D_H

struct Vector2D {
    float x;
    float y;

    Vector2D();
    Vector2D(float x, float y);

    Vector2D& operator+=(const Vector2D& v);
    Vector2D operator+(const Vector2D& v) const;
    Vector2D operator-(const Vector2D& v) const;
    Vector2D operator*(float scalar) const;

    float magnitude() const;
    Vector2D normalize() const;
};

#endif // VECTOR2D_H
