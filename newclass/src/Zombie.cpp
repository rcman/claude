#include "Zombie.h"

Zombie::Zombie(Vector2D pos, SDL_Texture* tex, ZombieType t) :
    Entity(pos, tex, TILE_SIZE, TILE_SIZE),
    type(t), lastAttackTime(0), exploded(false) {
    switch (type) {
        case NORMAL:
            health = 50;
            damage = 10;
            speed = 2.0f;
            detectionRange = 200.0f;
            attackRange = 40.0f;
            attackCooldown = 1000;
            break;
        case RUNNER:
            health = 30;
            damage = 5;
            speed = 3.5f;
            detectionRange = 250.0f;
            attackRange = 35.0f;
            attackCooldown = 800;
            break;
        case TANK:
            health = 100;
            damage = 15;
            speed = 1.5f;
            detectionRange = 150.0f;
            attackRange = 45.0f;
            attackCooldown = 1500;
            break;
        case EXPLODER:
            health = 40;
            damage = 30;
            speed = 2.5f;
            detectionRange = 180.0f;
            attackRange = 50.0f;
            attackCooldown = 0;
            break;
    }
}

void Zombie::update(Player& player) {
    if (!active || exploded) return;

    Vector2D playerPos = player.getPosition();
    float dist = distance(position.x, position.y, playerPos.x, playerPos.y);

    std::cout << "Zombie type " << type << " at (" << position.x << ", " << position.y
              << "), Distance to player: " << dist << std::endl;

    if (dist <= detectionRange) {
        Vector2D direction = (playerPos - position).normalize();
        velocity = direction * speed;
        std::cout << "Zombie pursuing player, Speed: " << speed << std::endl;
    } else {
        if (rand() % 100 < 5) {
            velocity.x = (rand() % 3 - 1) * speed * 0.5f;
            velocity.y = (rand() % 3 - 1) * speed * 0.5f;
            std::cout << "Zombie random movement: ("
                      << velocity.x << ", " << velocity.y << ")" << std::endl;
        }
    }

    if (dist <= attackRange) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastAttackTime >= attackCooldown) {
            attack(player);
            lastAttackTime = currentTime;
        }
    }

    Entity::update();
}

void Zombie::attack(Player& player) {
    if (type == EXPLODER && !exploded) {
        exploded = true;
        player.takeDamage(damage);
        active = false;
    } else {
        player.takeDamage(damage);
    }
}

void Zombie::takeDamage(int amount) {
    health -= amount;
    if (health <= 0) {
        active = false;
    }
}

ZombieType Zombie::getType() const { return type; }
int Zombie::getHealth() const { return health; }
int Zombie::getDamage() const { return damage; }
