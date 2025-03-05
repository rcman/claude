#include "Camera.h"

Camera::Camera(int mapWidth, int mapHeight) : mapWidth(mapWidth), mapHeight(mapHeight) {
    viewport.x = 0;
    viewport.y = 0;
    viewport.w = SCREEN_WIDTH;
    viewport.h = SCREEN_HEIGHT;
}

void Camera::update(const Vector2D& target) {
    viewport.x = static_cast<int>(target.x - SCREEN_WIDTH / 2);
    viewport.y = static_cast<int>(target.y - SCREEN_HEIGHT / 2);

    if (viewport.x < 0) viewport.x = 0;
    if (viewport.y < 0) viewport.y = 0;
    if (viewport.x > mapWidth - viewport.w) viewport.x = mapWidth - viewport.w;
    if (viewport.y > mapHeight - viewport.h) viewport.y = mapHeight - viewport.h;
}

SDL_Rect Camera::getViewport() const {
    return viewport;
}

bool Camera::isVisible(const Rectangle& rect) const {
    return (rect.x + rect.w > viewport.x &&
            rect.x < viewport.x + viewport.w &&
            rect.y + rect.h > viewport.y &&
            rect.y < viewport.y + viewport.h);
}

Vector2D Camera::worldToScreen(const Vector2D& worldPos) const {
    return Vector2D(worldPos.x - viewport.x, worldPos.y - viewport.y);
}

Vector2D Camera::screenToWorld(const Vector2D& screenPos) const {
    return Vector2D(screenPos.x + viewport.x, screenPos.y + viewport.y);
}
