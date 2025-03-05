#ifndef PLAYER_H
#define PLAYER_H

#include "Entity.h"
#include "Inventory.h"
#include "Building.h" // Forward declaration won’t suffice due to pointer usage

class Player : public Entity {
private:
    int health;
    int maxHealth;
    int armor;
    int weaponPower;
    Direction facing;
    Inventory inventory;
    Building* homeBase;
    bool isInside;

public:
    Player(Vector2D pos, SDL_Texture* tex);
    void update() override;
    void handleInput(const Uint8* keystates);
    void takeDamage(int amount);
    void heal(int amount);

    int getHealth() const;
    int getMaxHealth() const;
    int getArmor() const;
    int getWeaponPower() const;
    Direction getFacing() const;
    Inventory& getInventory();
    Building* getHomeBase() const;
    bool getIsInside() const;

    void setHomeBase(Building* building);
    void setIsInside(bool inside);
    void upgradeWeapon(int amount);
    void upgradeArmor(int amount);
    void upgradeMaxHealth(int amount);
};

#endif // PLAYER_H
