#ifndef CAMERA_H
#define CAMERA_H

#include "Common.h"
#include "Vector2D.h"
#include "Rectangle.h"

class Camera {
private:
    SDL_Rect viewport;
    int mapWidth, mapHeight;

public:
    Camera(int mapWidth, int mapHeight);
    void update(const Vector2D& target);
    SDL_Rect getViewport() const;
    bool isVisible(const Rectangle& rect) const;
    Vector2D worldToScreen(const Vector2D& worldPos) const;
    Vector2D screenToWorld(const Vector2D& screenPos) const;
};

#endif // CAMERA_H
