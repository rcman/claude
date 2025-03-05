#include "Entity.h"

Entity::Entity(Vector2D pos, SDL_Texture* tex, int w, int h) :
    position(pos), velocity(0, 0), texture(tex), active(true) {
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = w;
    srcRect.h = h;
    destRect.w = w;
    destRect.h = h;
    collider.w = w;
    collider.h = h;
    updateCollider();
}

Entity::~Entity() {}

void Entity::update() {
    position += velocity;
    updateCollider();
}

void Entity::render(SDL_Renderer* renderer, const Camera& camera) {
    if (!active || !texture) {
        if (!texture) {
            std::cout << "Warning: Entity at (" << position.x << ", " << position.y << ") has nullptr texture." << std::endl;
        }
        return;
    }

    Vector2D screenPos = camera.worldToScreen(position);
    destRect.x = static_cast<int>(screenPos.x);
    destRect.y = static_cast<int>(screenPos.y);

    SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
}

void Entity::updateCollider() {
    collider.x = position.x;
    collider.y = position.y;
}

bool Entity::isActive() const { return active; }
void Entity::setActive(bool a) { active = a; }

Vector2D Entity::getPosition() const { return position; }
void Entity::setPosition(const Vector2D& pos) { position = pos; updateCollider(); }

Vector2D Entity::getVelocity() const { return velocity; }
void Entity::setVelocity(const Vector2D& vel) { velocity = vel; }

Rectangle Entity::getCollider() const { return collider; }

bool Entity::checkCollision(const Entity& other) const {
    return collider.intersects(other.getCollider());
}
