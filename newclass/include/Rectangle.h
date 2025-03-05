#ifndef RECTANGLE_H
#define RECTANGLE_H

struct Rectangle {
    float x, y, w, h;

    Rectangle();
    Rectangle(float x, float y, float w, float h);

    bool intersects(const Rectangle& other) const;
    bool contains(float px, float py) const;
};

#endif // RECTANGLE_H
