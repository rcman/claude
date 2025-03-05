#include "Player.h"

Player::Player(Vector2D pos, SDL_Texture* tex) :
    Entity(pos, tex, TILE_SIZE, TILE_SIZE),
    health(100), maxHealth(100), armor(0), weaponPower(10),
    facing(DOWN), homeBase(nullptr), isInside(false) {}

void Player::update() {
    std::cout << "Player position before update: ("
              << position.x << ", " << position.y << ")" << std::endl;

    Entity::update();

    if (position.x < 0) position.x = 0;
    if (position.y < 0) position.y = 0;
    if (position.x > MAP_WIDTH - collider.w) position.x = MAP_WIDTH - collider.w;
    if (position.y > MAP_HEIGHT - collider.h) position.y = MAP_HEIGHT - collider.h;

    std::cout << "Player position after update: ("
              << position.x << ", " << position.y << "), Health: "
              << health << "/" << maxHealth << std::endl;
}

void Player::handleInput(const Uint8* keystates) {
    velocity = Vector2D(0, 0);

    if (keystates[SDL_SCANCODE_W]) {
        velocity.y = -PLAYER_SPEED;
        facing = UP;
    }
    if (keystates[SDL_SCANCODE_S]) {
        velocity.y = PLAYER_SPEED;
        facing = DOWN;
    }
    if (keystates[SDL_SCANCODE_A]) {
        velocity.x = -PLAYER_SPEED;
        facing = LEFT;
    }
    if (keystates[SDL_SCANCODE_D]) {
        velocity.x = PLAYER_SPEED;
        facing = RIGHT;
    }

    if (velocity.x != 0 && velocity.y != 0) {
        velocity = velocity.normalize() * PLAYER_SPEED;
    }
}

void Player::takeDamage(int amount) {
    int actualDamage = std::max(1, amount - armor / 10);
    health -= actualDamage;
    if (health < 0) health = 0;
}

void Player::heal(int amount) {
    health = std::min(health + amount, maxHealth);
}

int Player::getHealth() const { return health; }
int Player::getMaxHealth() const { return maxHealth; }
int Player::getArmor() const { return armor; }
int Player::getWeaponPower() const { return weaponPower; }
Direction Player::getFacing() const { return facing; }
Inventory& Player::getInventory() { return inventory; }
Building* Player::getHomeBase() const { return homeBase; }
bool Player::getIsInside() const { return isInside; }

void Player::setHomeBase(Building* building) { homeBase = building; }
void Player::setIsInside(bool inside) { isInside = inside; }

void Player::upgradeWeapon(int amount) { weaponPower += amount; }
void Player::upgradeArmor(int amount) { armor += amount; }
void Player::upgradeMaxHealth(int amount) { maxHealth += amount; health = std::min(health + amount, maxHealth); }
