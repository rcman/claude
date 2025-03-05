#ifndef ENTITY_H
#define ENTITY_H

#include "Common.h"
#include "Vector2D.h"
#include "Rectangle.h"
#include "Camera.h"

class Entity {
protected:
    Vector2D position;
    Vector2D velocity;
    Rectangle collider;
    SDL_Texture* texture;
    SDL_Rect srcRect;
    SDL_Rect destRect;
    bool active;

public:
    Entity(Vector2D pos, SDL_Texture* tex, int w, int h);
    virtual ~Entity();
    virtual void update();
    virtual void render(SDL_Renderer* renderer, const Camera& camera);

    void updateCollider();
    bool isActive() const;
    void setActive(bool a);
    Vector2D getPosition() const;
    void setPosition(const Vector2D& pos);
    Vector2D getVelocity() const;
    void setVelocity(const Vector2D& vel);
    Rectangle getCollider() const;
    bool checkCollision(const Entity& other) const;
};

#endif // ENTITY_H
