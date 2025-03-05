#include "Rectangle.h"

Rectangle::Rectangle() : x(0), y(0), w(0), h(0) {}
Rectangle::Rectangle(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

bool Rectangle::intersects(const Rectangle& other) const {
    return (x < other.x + other.w &&
            x + w > other.x &&
            y < other.y + other.h &&
            y + h > other.y);
}

bool Rectangle::contains(float px, float py) const {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}
