#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "Entity.h"
#include "Player.h"

class Zombie : public Entity {
private:
    ZombieType type;
    int health;
    int damage;
    float speed;
    float detectionRange;
    float attackRange;
    Uint32 lastAttackTime;
    Uint32 attackCooldown;
    bool exploded;

public:
    Zombie(Vector2D pos, SDL_Texture* tex, ZombieType t);
    void update(Player& player);
    void attack(Player& player);
    void takeDamage(int amount);

    ZombieType getType() const;
    int getHealth() const;
    int getDamage() const;
};

#endif // ZOMBIE_H
