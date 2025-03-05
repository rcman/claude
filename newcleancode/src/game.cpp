#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <random>
#include <ctime>
#include <algorithm>
#include <memory>
#include <functional>
#include <cmath>
#include <fstream>
#include <sstream>

// Constants
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int MAP_WIDTH = 2000;
const int MAP_HEIGHT = 2000;
const int TILE_SIZE = 32;
const int PLAYER_SPEED = 5;
const int MAX_FPS = 60;
const int FRAME_DELAY = 1000 / MAX_FPS;
const int DAY_NIGHT_CYCLE_DURATION = 600000; // 10 minutes in milliseconds
const float DAY_RATIO = 0.7f; // 70% day, 30% night

// Game States
enum GameState {
    MAIN_MENU,
    GAMEPLAY,
    INVENTORY,
    CRAFTING,
    GAME_OVER,
    PAUSE
};

// Item Types
enum ItemType {
    WEAPON,
    ARMOR,
    HEALTH,
    RESOURCE_WOOD,
    RESOURCE_METAL,
    RESOURCE_FOOD,
    RESOURCE_CLOTH,
    AMMO
};

// Zombie Types
enum ZombieType {
    NORMAL,
    RUNNER,
    TANK,
    EXPLODER
};

// Time of Day
enum TimeOfDay {
    DAY,
    NIGHT
};

// Direction
enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
};

// Forward declarations
class Entity;
class Player;
class Zombie;
class Item;
class Building;
class Game;

// Utility functions
float distance(float x1, float y1, float x2, float y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

// Vector2D struct for positions and velocities
struct Vector2D {
    float x;
    float y;

    Vector2D() : x(0), y(0) {}
    Vector2D(float x, float y) : x(x), y(y) {}

    Vector2D& operator+=(const Vector2D& v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    Vector2D operator+(const Vector2D& v) const {
        return Vector2D(x + v.x, y + v.y);
    }

    Vector2D operator-(const Vector2D& v) const {
        return Vector2D(x - v.x, y - v.y);
    }

    Vector2D operator*(float scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }

    float magnitude() const {
        return std::sqrt(x * x + y * y);
    }

    Vector2D normalize() const {
        float mag = magnitude();
        if (mag > 0) {
            return Vector2D(x / mag, y / mag);
        }
        return Vector2D(0, 0);
    }
};

// Rectangle struct for collision detection
struct Rectangle {
    float x, y, w, h;

    Rectangle() : x(0), y(0), w(0), h(0) {}
    Rectangle(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    bool intersects(const Rectangle& other) const {
        return (x < other.x + other.w &&
                x + w > other.x &&
                y < other.y + other.h &&
                y + h > other.y);
    }

    bool contains(float px, float py) const {
        return (px >= x && px <= x + w && py >= y && py <= y + h);
    }
};

// Camera class to handle viewport
class Camera {
private:
    SDL_Rect viewport;
    int mapWidth, mapHeight;

public:
    Camera(int mapWidth, int mapHeight) : mapWidth(mapWidth), mapHeight(mapHeight) {
        viewport.x = 0;
        viewport.y = 0;
        viewport.w = SCREEN_WIDTH;
        viewport.h = SCREEN_HEIGHT;
    }

    void update(const Vector2D& target) {
        viewport.x = static_cast<int>(target.x - SCREEN_WIDTH / 2);
        viewport.y = static_cast<int>(target.y - SCREEN_HEIGHT / 2);

        if (viewport.x < 0) viewport.x = 0;
        if (viewport.y < 0) viewport.y = 0;
        if (viewport.x > mapWidth - viewport.w) viewport.x = mapWidth - viewport.w;
        if (viewport.y > mapHeight - viewport.h) viewport.y = mapHeight - viewport.h;
    }

    SDL_Rect getViewport() const {
        return viewport;
    }

    bool isVisible(const Rectangle& rect) const {
        return (rect.x + rect.w > viewport.x &&
                rect.x < viewport.x + viewport.w &&
                rect.y + rect.h > viewport.y &&
                rect.y < viewport.y + viewport.h);
    }

    Vector2D worldToScreen(const Vector2D& worldPos) const {
        return Vector2D(worldPos.x - viewport.x, worldPos.y - viewport.y);
    }

    Vector2D screenToWorld(const Vector2D& screenPos) const {
        return Vector2D(screenPos.x + viewport.x, screenPos.y + viewport.y);
    }
};

// Base Entity class
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
    Entity(Vector2D pos, SDL_Texture* tex, int w, int h) :
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

    virtual ~Entity() {}

    virtual void update() {
        position += velocity;
        updateCollider();
    }

    void updateCollider() {
        collider.x = position.x;
        collider.y = position.y;
    }

    virtual void render(SDL_Renderer* renderer, const Camera& camera) {
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

    bool isActive() const { return active; }
    void setActive(bool a) { active = a; }

    Vector2D getPosition() const { return position; }
    void setPosition(const Vector2D& pos) { position = pos; updateCollider(); }

    Vector2D getVelocity() const { return velocity; }
    void setVelocity(const Vector2D& vel) { velocity = vel; }

    Rectangle getCollider() const { return collider; }

    bool checkCollision(const Entity& other) const {
        return collider.intersects(other.getCollider());
    }
};

// Item class
class Item : public Entity {
private:
    ItemType type;
    int value;
    std::string name;

public:
    Item(Vector2D pos, SDL_Texture* tex, ItemType t, int val, const std::string& n) :
        Entity(pos, tex, TILE_SIZE, TILE_SIZE), type(t), value(val), name(n) {}

    ItemType getType() const { return type; }
    int getValue() const { return value; }
    std::string getName() const { return name; }
};

// Inventory class
class Inventory {
private:
    std::map<ItemType, int> items;
    int capacity;

public:
    Inventory(int cap = 20) : capacity(cap) {
        items[WEAPON] = 0;
        items[ARMOR] = 0;
        items[HEALTH] = 0;
        items[RESOURCE_WOOD] = 0;
        items[RESOURCE_METAL] = 0;
        items[RESOURCE_FOOD] = 0;
        items[RESOURCE_CLOTH] = 0;
        items[AMMO] = 0;
    }

    bool addItem(const Item& item) {
        if (getTotalItems() >= capacity) return false;

        items[item.getType()] += item.getValue();
        return true;
    }

    bool useItem(ItemType type, int amount = 1) {
        if (items[type] < amount) return false;

        items[type] -= amount;
        return true;
    }

    int getItemCount(ItemType type) const {
        auto it = items.find(type);
        if (it != items.end()) {
            return it->second;
        }
        return 0;
    }

    int getTotalItems() const {
        int total = 0;
        for (const auto& item : items) {
            total += item.second;
        }
        return total;
    }

    void transferTo(Inventory& other, ItemType type, int amount) {
        int available = std::min(amount, items[type]);
        if (available <= 0) return;

        if (other.getTotalItems() + available <= other.capacity) {
            items[type] -= available;
            other.items[type] += available;
        }
    }
};

// Player class
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
    Player(Vector2D pos, SDL_Texture* tex) :
        Entity(pos, tex, TILE_SIZE, TILE_SIZE),
        health(100), maxHealth(100), armor(0), weaponPower(10),
        facing(DOWN), homeBase(nullptr), isInside(false) {}

    void update() override {
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

    void handleInput(const Uint8* keystates) {
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

    void takeDamage(int amount) {
        int actualDamage = std::max(1, amount - armor / 10);
        health -= actualDamage;
        if (health < 0) health = 0;
    }

    void heal(int amount) {
        health = std::min(health + amount, maxHealth);
    }

    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getArmor() const { return armor; }
    int getWeaponPower() const { return weaponPower; }
    Direction getFacing() const { return facing; }
    Inventory& getInventory() { return inventory; }
    Building* getHomeBase() const { return homeBase; }
    bool getIsInside() const { return isInside; }

    void setHomeBase(Building* building) { homeBase = building; }
    void setIsInside(bool inside) { isInside = inside; }

    void upgradeWeapon(int amount) { weaponPower += amount; }
    void upgradeArmor(int amount) { armor += amount; }
    void upgradeMaxHealth(int amount) { maxHealth += amount; health = std::min(health + amount, maxHealth); }
};

// Zombie class
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
    Zombie(Vector2D pos, SDL_Texture* tex, ZombieType t) :
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

    void update(Player& player) {
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

    void attack(Player& player) {
        if (type == EXPLODER && !exploded) {
            exploded = true;
            player.takeDamage(damage);
            active = false;
        } else {
            player.takeDamage(damage);
        }
    }

    void takeDamage(int amount) {
        health -= amount;
        if (health <= 0) {
            active = false;
        }
    }

    ZombieType getType() const { return type; }
    int getHealth() const { return health; }
    int getDamage() const { return damage; }
};

// Building class
class Building : public Entity {
private:
    std::vector<Item*> items;
    bool isPlayerHome;
    Rectangle entrance;
    Rectangle interior;
    Inventory storage;

public:
    Building(Vector2D pos, SDL_Texture* tex, int w, int h) :
        Entity(pos, tex, w, h), isPlayerHome(false) {

        entrance.x = position.x + w / 2 - TILE_SIZE / 2;
        entrance.y = position.y + h - TILE_SIZE;
        entrance.w = TILE_SIZE;
        entrance.h = TILE_SIZE;

        interior.x = position.x;
        interior.y = position.y;
        interior.w = w;
        interior.h = h;
    }

    bool isAtEntrance(const Vector2D& pos) const {
        return entrance.contains(pos.x, pos.y);
    }

    bool isInside(const Vector2D& pos) const {
        return interior.contains(pos.x, pos.y);
    }

    void addItem(Item* item) {
        items.push_back(item);
    }

    Item* getItem(int index) {
        if (index >= 0 && index < items.size()) {
            Item* item = items[index];
            items.erase(items.begin() + index);
            return item;
        }
        return nullptr;
    }

    std::vector<Item*>& getItems() {
        return items;
    }

    bool isHomeBase() const { return isPlayerHome; }
    void setHomeBase(bool home) { isPlayerHome = home; }

    Inventory& getStorage() { return storage; }
};

// TileMap class for the game world
class TileMap {
private:
    std::vector<std::vector<int>> map;
    SDL_Texture* tileset;
    std::vector<SDL_Rect> tileRects;
    int mapWidth, mapHeight;
    int tileSize;

public:
    TileMap(SDL_Texture* tiles, int width, int height, int tileSize) :
        tileset(tiles), mapWidth(width), mapHeight(height), tileSize(tileSize) {

        map.resize(mapHeight / tileSize);
        for (auto& row : map) {
            row.resize(mapWidth / tileSize, 0);
        }

        int tilesetWidth, tilesetHeight;
        SDL_QueryTexture(tileset, nullptr, nullptr, &tilesetWidth, &tilesetHeight);

	printf("Value is %d, Value is %d/n", tilesetWidth, tilesetHeight);

        for (int y = 0; y < tilesetHeight; y += tileSize) {
            for (int x = 0; x < tilesetWidth; x += tileSize) {
                SDL_Rect rect = { x, y, tileSize, tileSize };
                tileRects.push_back(rect);
            }
        }

        generateTerrain();
    }

    void generateTerrain() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> tileTypeDist(0, tileRects.size() - 1);
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

        for (int y = 0; y < map.size(); ++y) {
            for (int x = 0; x < map[y].size(); ++x) {
                float prob = probDist(gen);

                if (prob < 0.01f) {
                    map[y][x] = 2;
                } else if (prob < 0.1f) {
                    map[y][x] = 1;
                }
            }
        }
    }

    void render(SDL_Renderer* renderer, const Camera& camera) {
        if (!tileset) {
            std::cout << "Warning: TileMap has nullptr tileset texture." << std::endl;
            return;
        }

        SDL_Rect viewport = camera.getViewport();

        int startX = viewport.x / tileSize;
        int startY = viewport.y / tileSize;
        int endX = (viewport.x + viewport.w) / tileSize + 1;
        int endY = (viewport.y + viewport.h) / tileSize + 1;

        startX = std::max(0, startX);
        startY = std::max(0, startY);
        endX = std::min(static_cast<int>(map[0].size()), endX);
        endY = std::min(static_cast<int>(map.size()), endY);

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                int tileIndex = map[y][x];
                SDL_Rect destRect = {
                    x * tileSize - viewport.x,
                    y * tileSize - viewport.y,
                    tileSize,
                    tileSize
                };
                SDL_RenderCopy(renderer, tileset, &tileRects[tileIndex], &destRect);
            }
        }
    }

    bool isObstacle(int x, int y) const {
        if (x < 0 || y < 0 || y >= map.size() || x >= map[y].size()) {
            return true;
        }
        return map[y][x] == 2;
    }

    bool checkCollision(const Rectangle& rect) const {
        int startTileX = static_cast<int>(rect.x) / tileSize;
        int startTileY = static_cast<int>(rect.y) / tileSize;
        int endTileX = static_cast<int>(rect.x + rect.w) / tileSize;
        int endTileY = static_cast<int>(rect.y + rect.h) / tileSize;

        for (int y = startTileY; y <= endTileY; ++y) {
            for (int x = startTileX; x <= endTileX; ++x) {
                if (isObstacle(x, y)) {
                    std::cout << "Collision detected at tile (" << x << ", " << y
                              << ") for rect at (" << rect.x << ", " << rect.y << ")" << std::endl;
                    return true;
                }
            }
        }
        return false;
    }
};

// UIManager class for handling game UI
class UIManager {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* hpBarTexture;
    SDL_Texture* inventoryTexture;
    GameState& gameState;
    Player& player;
    TimeOfDay& timeOfDay;

public:
    UIManager(SDL_Renderer* ren, TTF_Font* f, SDL_Texture* hpBar, SDL_Texture* invTex, GameState& state, Player& p, TimeOfDay& time) :
        renderer(ren), font(f), hpBarTexture(hpBar), inventoryTexture(invTex), gameState(state), player(p), timeOfDay(time) {}

    void renderText(const std::string& text, int x, int y, SDL_Color color) {
        if (!font) {
            std::cout << "Warning: UIManager has nullptr font, skipping text render." << std::endl;
            return;
        }

        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (!surface) {
            std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
            SDL_FreeSurface(surface);
            return;
        }

        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        SDL_Rect destRect = { x, y, w, h };

        SDL_RenderCopy(renderer, texture, nullptr, &destRect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    void renderHealthBar() {
        int health = player.getHealth();
        int maxHealth = player.getMaxHealth();

        SDL_Rect bgRect = { 20, 20, 200, 30 };
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &bgRect);

        SDL_Rect fillRect = { 20, 20, static_cast<int>(200.0f * health / maxHealth), 30 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &fillRect);

        SDL_Color white = { 255, 255, 255, 255 };
        renderText("HP: " + std::to_string(health) + "/" + std::to_string(maxHealth), 25, 25, white);
    }

    void renderInventoryPreview() {
        SDL_Color white = { 255, 255, 255, 255 };

        Inventory& inv = player.getInventory();
        renderText("Wood: " + std::to_string(inv.getItemCount(RESOURCE_WOOD)), 20, 60, white);
        renderText("Metal: " + std::to_string(inv.getItemCount(RESOURCE_METAL)), 20, 85, white);
        renderText("Food: " + std::to_string(inv.getItemCount(RESOURCE_FOOD)), 20, 110, white);
        renderText("Cloth: " + std::to_string(inv.getItemCount(RESOURCE_CLOTH)), 20, 135, white);
        renderText("Ammo: " + std::to_string(inv.getItemCount(AMMO)), 20, 160, white);
    }

    void renderTimeOfDay() {
        SDL_Color color;
        std::string timeText;

        if (timeOfDay == DAY) {
            color = { 255, 255, 0, 255 };
            timeText = "Day";
        } else {
            color = { 100, 100, 255, 255 };
            timeText = "Night";
        }

        renderText(timeText, SCREEN_WIDTH - 100, 20, color);
    }

    void renderGameplay() {
        renderHealthBar();
        renderInventoryPreview();
        renderTimeOfDay();

        if (player.getHomeBase()) {
            SDL_Color green = { 0, 255, 0, 255 };
            renderText("Home Base Set", SCREEN_WIDTH - 200, 50, green);
        }

        if (player.getIsInside()) {
            SDL_Color cyan = { 0, 255, 255, 255 };
            renderText("Inside Building", SCREEN_WIDTH - 200, 80, cyan);
        }
    }

    void renderInventoryScreen() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderFillRect(renderer, &bgRect);

        SDL_Color white = { 255, 255, 255, 255 };
        renderText("Inventory", SCREEN_WIDTH / 2 - 100, 50, white);

        Inventory& inv = player.getInventory();
        int y = 120;

        //renderText("Resources:", 300, y, white); y += 40;
        //renderText("Wood: " + std::to_string(inv.getItemCount(RESOURCE_WOOD)), 320, y, white); y += 30;
        //renderText("Metal: " + std::to_string(inv.getItemCount(RESOURCE_METAL)), 320, y, white); y += 30;
        //renderText("Food: " + std::to_string(inv.getItemCount(RESOURCE_FOOD)), 320, y, white); y += 30;
        //renderText("Cloth: " + std::to_string(inv.getItemCount(RESOURCE_CLOTH)), 320, y, white); y += 50;

        //renderText("Combat:", 300, y, white); y += 40;
        //renderText("Weapon Power: " + std::to_string(player.getWeaponPower()), 320, y, white); y += 30;
        //renderText("Armor: " + std::to_string(player.getArmor()), 320, y, white); y += 30;
        //renderText("Ammo: " + std::to_string(inv.getItemCount(AMMO)), 320, y, white); y += 30;
        //renderText("Health Items: " + std::to_string(inv.getItemCount(HEALTH)), 320, y, white); y += 50;

        if (player.getHomeBase() && player.getIsInside()) {
            //renderText("Home Base Storage:", SCREEN_WIDTH - 500, 120, white);
            Inventory& storage = player.getHomeBase()->getStorage();

            y = 160;
            //renderText("Wood: " + std::to_string(storage.getItemCount(RESOURCE_WOOD)), SCREEN_WIDTH - 480, y, white); y += 30;
            //renderText("Metal: " + std::to_string(storage.getItemCount(RESOURCE_METAL)), SCREEN_WIDTH - 480, y, white); y += 30;
            //renderText("Food: " + std::to_string(storage.getItemCount(RESOURCE_FOOD)), SCREEN_WIDTH - 480, y, white); y += 30;
            //renderText("Cloth: " + std::to_string(storage.getItemCount(RESOURCE_CLOTH)), SCREEN_WIDTH - 480, y, white); y += 30;
            renderText("Ammo: " + std::to_string(storage.getItemCount(AMMO)), SCREEN_WIDTH - 480, y, white); y += 30;
            //renderText("Health Items: " + std::to_string(storage.getItemCount(HEALTH)), SCREEN_WIDTH - 480, y, white); y += 50;

          //  renderText("Press [T] to transfer items to/from storage", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 100, white);
        }

        //renderText("Press [I] to close inventory", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT - 50, white);
    }

    void renderCraftingScreen() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderFillRect(renderer, &bgRect);

        SDL_Color white = { 255, 255, 255, 255 };
        //renderText("Crafting", SCREEN_WIDTH / 2 - 80, 50, white);

        Inventory& inv = player.getInventory();
        int y = 120;

        //renderText("Resources:", 200, y, white); y += 40;
        //renderText("Wood: " + std::to_string(inv.getItemCount(RESOURCE_WOOD)), 220, y, white); y += 30;
        //renderText("Metal: " + std::to_string(inv.getItemCount(RESOURCE_METAL)), 220, y, white); y += 30;
        //renderText("Food: " + std::to_string(inv.getItemCount(RESOURCE_FOOD)), 220, y, white); y += 30;
        //renderText("Cloth: " + std::to_string(inv.getItemCount(RESOURCE_CLOTH)), 220, y, white); y += 50;

        y = 120;
        //renderText("Crafting Options:", SCREEN_WIDTH - 600, y, white); y += 40;

        //renderText("1. Upgrade Weapon (5 Metal, 3 Wood) [Press 1]", SCREEN_WIDTH - 580, y, white); y += 30;
        //renderText("2. Upgrade Armor (8 Cloth, 2 Metal) [Press 2]", SCREEN_WIDTH - 580, y, white); y += 30;
        //renderText("3. Craft Health Pack (5 Food, 2 Cloth) [Press 3]", SCREEN_WIDTH - 580, y, white); y += 30;
        //renderText("4. Craft Ammo (3 Metal, 2 Wood) [Press 4]", SCREEN_WIDTH - 580, y, white); y += 30;

        //renderText("Press [C] to close crafting menu", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT - 50, white);
    }

    void renderPauseScreen() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderFillRect(renderer, &bgRect);

        SDL_Color white = { 255, 255, 255, 255 };
        //renderText("GAME PAUSED", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 100, white);
        //renderText("Press [P] to resume", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2, white);
        //renderText("Press [ESC] to quit", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 + 50, white);
    }

    void renderGameOverScreen() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderFillRect(renderer, &bgRect);

        SDL_Color red = { 255, 0, 0, 255 };
        SDL_Color white = { 255, 255, 255, 255 };

       // renderText("GAME OVER", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, red);
       // renderText("Press [R] to restart", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50, white);
       // renderText("Press [ESC] to quit", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 100, white);
    }

    void renderMainMenu() {
       // SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
       // SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
       // SDL_RenderFillRect(renderer, &bgRect);

       // SDL_Color title = { 255, 200, 0, 255 };
       // SDL_Color normal = { 200, 200, 200, 255 };

       // renderText("ZOMBIE SURVIVAL", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 4, title);
       // renderText("Press [ENTER] to start game", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2, normal);
      //  renderText("Press [ESC] to quit", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50, normal);

       // int y = SCREEN_HEIGHT - 200;
        //renderText("Controls:", 100, y, normal); y += 30;
        //renderText("WASD - Move", 120, y, normal); y += 25;
        //renderText("E - Interact with buildings/items", 120, y, normal); y += 25;
        //renderText("I - Inventory", 120, y, normal); y += 25;
        //renderText("C - Crafting", 120, y, normal); y += 25;
        //renderText("H - Set current building as home base", 120, y, normal);
    }

    void render() {
        switch (gameState) {
            case MAIN_MENU:
                renderMainMenu();
                break;
            case GAMEPLAY:
                renderGameplay();
                break;
            case INVENTORY:
                renderGameplay();
                renderInventoryScreen();
                break;
            case CRAFTING:
                renderGameplay();
                renderCraftingScreen();
                break;
            case PAUSE:
                renderGameplay();
                renderPauseScreen();
                break;
            case GAME_OVER:
                renderGameOverScreen();
                break;
        }
    }
};

// Game class - main game logic
class Game {
private:
    bool running;
    SDL_Window* window;
    SDL_Renderer* renderer;
    GameState gameState;
    TimeOfDay timeOfDay;
    Uint32 gameTime;
    Uint32 lastFrameTime;
    Uint32 lastTimeUpdate;
    Camera camera;

    SDL_Texture* playerTexture;
    SDL_Texture* zombieTextures[4];
    SDL_Texture* buildingTextures[3];
    SDL_Texture* itemTextures[8];
    SDL_Texture* tilesetTexture;
    SDL_Texture* uiTextures[2];
    TTF_Font* font;

    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Zombie>> zombies;
    std::vector<std::unique_ptr<Building>> buildings;
    std::vector<std::unique_ptr<Item>> items;
    std::unique_ptr<TileMap> tileMap;
    std::unique_ptr<UIManager> uiManager;

    std::mt19937 rng;

    Uint32 lastZombieSpawn;
    Uint32 zombieSpawnInterval;

public:
    Game() :
        running(true),
        window(nullptr),
        renderer(nullptr),
        gameState(GAMEPLAY),
       // gameState(MAIN_MENU),
        timeOfDay(DAY),
        gameTime(0),
        lastFrameTime(0),
        lastTimeUpdate(0),
        camera(MAP_WIDTH, MAP_HEIGHT),
        lastZombieSpawn(0),
        zombieSpawnInterval(5000),
        playerTexture(nullptr),
        tilesetTexture(nullptr),
        font(nullptr) {
        std::random_device rd;
        rng = std::mt19937(rd());
        for (int i = 0; i < 4; ++i) zombieTextures[i] = nullptr;
        for (int i = 0; i < 3; ++i) buildingTextures[i] = nullptr;
        for (int i = 0; i < 8; ++i) itemTextures[i] = nullptr;
        for (int i = 0; i < 2; ++i) uiTextures[i] = nullptr;
    }

    ~Game() {
        clean();
    }

    bool init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
        int flags = 0;
        if (fullscreen) {
            flags = SDL_WINDOW_FULLSCREEN;
        }

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return false;
        }
        std::cout << "SDL initialized successfully" << std::endl;

        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            std::cerr << "SDL_image initialization failed: " << IMG_GetError() << std::endl;
            return false;
        }

        if (TTF_Init() != 0) {
            std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        std::cout << "Game initialized - Window: " << width << "x" << height
                  << ", Fullscreen: " << (fullscreen ? "Yes" : "No") << std::endl;

        running = true;
        return loadMedia();
    }

    bool loadMedia() {
        playerTexture = createColorTexture(TILE_SIZE, TILE_SIZE, 0, 0, 255, 255);
        if (!playerTexture) return false;

        zombieTextures[NORMAL] = createColorTexture(TILE_SIZE, TILE_SIZE, 0, 255, 0, 255);
        if (!zombieTextures[NORMAL]) return false;
        zombieTextures[RUNNER] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 255, 0, 255);
        if (!zombieTextures[RUNNER]) return false;
        zombieTextures[TANK] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 128, 0, 255);
        if (!zombieTextures[TANK]) return false;
        zombieTextures[EXPLODER] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 0, 0, 255);
        if (!zombieTextures[EXPLODER]) return false;

        buildingTextures[0] = createColorTexture(TILE_SIZE * 4, TILE_SIZE * 3, 128, 128, 128, 255);
        if (!buildingTextures[0]) return false;
        buildingTextures[1] = createColorTexture(TILE_SIZE * 6, TILE_SIZE * 4, 150, 150, 150, 255);
        if (!buildingTextures[1]) return false;
        buildingTextures[2] = createColorTexture(TILE_SIZE * 8, TILE_SIZE * 6, 170, 170, 170, 255);
        if (!buildingTextures[2]) return false;

        itemTextures[WEAPON] = createColorTexture(TILE_SIZE, TILE_SIZE, 192, 192, 0, 255);
        if (!itemTextures[WEAPON]) return false;
        itemTextures[ARMOR] = createColorTexture(TILE_SIZE, TILE_SIZE, 192, 0, 192, 255);
        if (!itemTextures[ARMOR]) return false;
        itemTextures[HEALTH] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 0, 0, 255);
        if (!itemTextures[HEALTH]) return false;
        itemTextures[RESOURCE_WOOD] = createColorTexture(TILE_SIZE, TILE_SIZE, 139, 69, 19, 255);
        if (!itemTextures[RESOURCE_WOOD]) return false;
        itemTextures[RESOURCE_METAL] = createColorTexture(TILE_SIZE, TILE_SIZE, 169, 169, 169, 255);
        if (!itemTextures[RESOURCE_METAL]) return false;
        itemTextures[RESOURCE_FOOD] = createColorTexture(TILE_SIZE, TILE_SIZE, 0, 128, 0, 255);
        if (!itemTextures[RESOURCE_FOOD]) return false;
        itemTextures[RESOURCE_CLOTH] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 255, 255, 255);
        if (!itemTextures[RESOURCE_CLOTH]) return false;
        itemTextures[AMMO] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 215, 0, 255);
        if (!itemTextures[AMMO]) return false;

        tilesetTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TILE_SIZE * 3, TILE_SIZE);
        if (!tilesetTexture) {
            std::cerr << "Tileset texture creation failed: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_SetRenderTarget(renderer, tilesetTexture);

        SDL_Rect rect = { 0, 0, TILE_SIZE, TILE_SIZE };
        SDL_SetRenderDrawColor(renderer, 100, 150, 100, 255);
        SDL_RenderFillRect(renderer, &rect);

        rect.x = TILE_SIZE;
        SDL_SetRenderDrawColor(renderer, 120, 170, 120, 255);
        SDL_RenderFillRect(renderer, &rect);

        rect.x = TILE_SIZE * 2;
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderTarget(renderer, nullptr);

        uiTextures[0] = createColorTexture(200, 30, 255, 0, 0, 255);
        if (!uiTextures[0]) return false;
        uiTextures[1] = createColorTexture(400, 300, 50, 50, 50, 200);
        if (!uiTextures[1]) return false;

        font = TTF_OpenFont("arial.ttf", 24);
        if (!font) {
            std::cerr << "Font loading failed, using fallback: " << TTF_GetError() << std::endl;
            return false;
        }
	setupGame();
	gameState=GAMEPLAY;
        return true;

    }

    SDL_Texture* createColorTexture(int width, int height, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if (!texture) {
            std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_Rect rect = { 0, 0, width, height };
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderTarget(renderer, nullptr);

        return texture;
    }

    void setupGame() {
        player = std::make_unique<Player>(Vector2D(MAP_WIDTH / 2, MAP_HEIGHT / 2), playerTexture);
        std::cout << "New game setup - Player spawned at ("
                  << MAP_WIDTH / 2 << ", " << MAP_HEIGHT / 2 << ")" << std::endl;

        tileMap = std::make_unique<TileMap>(tilesetTexture, MAP_WIDTH, MAP_HEIGHT, TILE_SIZE);

        createBuildings();

        for (int i = 0; i < 10; ++i) {
            spawnZombie();
        }

        for (int i = 0; i < 20; ++i) {
            spawnItem();
        }

        uiManager = std::make_unique<UIManager>(renderer, font, uiTextures[0], uiTextures[1], gameState, *player, timeOfDay);

        gameTime = 0;
        lastFrameTime = SDL_GetTicks();
        lastTimeUpdate = lastFrameTime;
        lastZombieSpawn = lastFrameTime;

        gameState = GAMEPLAY;
        timeOfDay = DAY;

        std::cout << "Game setup complete - Zombies: " << zombies.size()
                  << ", Items: " << items.size() << ", Buildings: "
                  << buildings.size() << std::endl;
    }

    void createBuildings() {
        std::uniform_int_distribution<int> xPosDist(TILE_SIZE * 5, MAP_WIDTH - TILE_SIZE * 10);
        std::uniform_int_distribution<int> yPosDist(TILE_SIZE * 5, MAP_HEIGHT - TILE_SIZE * 10);
        std::uniform_int_distribution<int> buildingTypeDist(0, 2);

        for (int i = 0; i < 15; ++i) {
            int type = buildingTypeDist(rng);
            int width, height;

            switch (type) {
                case 0: width = TILE_SIZE * 4; height = TILE_SIZE * 3; break;
                case 1: width = TILE_SIZE * 6; height = TILE_SIZE * 4; break;
                case 2: width = TILE_SIZE * 8; height = TILE_SIZE * 6; break;
            }

            bool validPosition = false;
            Vector2D pos;

            int attempts = 0;
            while (!validPosition && attempts < 20) {
                validPosition = true;
                pos.x = xPosDist(rng);
                pos.y = yPosDist(rng);

                Rectangle newRect(pos.x, pos.y, width, height);

                for (const auto& building : buildings) {
                    Rectangle existingRect = building->getCollider();

                    existingRect.x -= TILE_SIZE * 2;
                    existingRect.y -= TILE_SIZE * 2;
                    existingRect.w += TILE_SIZE * 4;
                    existingRect.h += TILE_SIZE * 4;

                    if (existingRect.intersects(newRect)) {
                        validPosition = false;
                        break;
                    }
                }

                attempts++;
            }

            if (validPosition) {
                auto building = std::make_unique<Building>(pos, buildingTextures[type], width, height);

                std::uniform_int_distribution<int> itemCountDist(1, 5);
                int itemCount = itemCountDist(rng);

                for (int j = 0; j < itemCount; ++j) {
                    createItemInBuilding(*building);
                }

                buildings.push_back(std::move(building));
            }
        }
    }

    void createItemInBuilding(Building& building) {
        std::uniform_int_distribution<int> itemTypeDist(0, 7);
        std::uniform_int_distribution<int> valueDist(1, 5);

        ItemType type = static_cast<ItemType>(itemTypeDist(rng));
        int value = valueDist(rng);
        std::string name;

        switch (type) {
            case WEAPON: name = "Weapon Upgrade"; break;
            case ARMOR: name = "Armor Piece"; break;
            case HEALTH: name = "Health Pack"; break;
            case RESOURCE_WOOD: name = "Wood"; break;
            case RESOURCE_METAL: name = "Metal"; break;
            case RESOURCE_FOOD: name = "Food"; break;
            case RESOURCE_CLOTH: name = "Cloth"; break;
            case AMMO: name = "Ammo"; break;
        }

        Item* item = new Item(Vector2D(0, 0), itemTextures[type], type, value, name);
        building.addItem(item);
    }

    void spawnZombie() {
        std::uniform_real_distribution<float> xPosDist(0, MAP_WIDTH - TILE_SIZE);
        std::uniform_real_distribution<float> yPosDist(0, MAP_HEIGHT - TILE_SIZE);
        std::uniform_int_distribution<int> typeDist(0, 3);

        Vector2D playerPos = player->getPosition();
        Vector2D pos;
        bool validPosition = false;

        while (!validPosition) {
            pos.x = xPosDist(rng);
            pos.y = yPosDist(rng);

            float dist = distance(pos.x, pos.y, playerPos.x, playerPos.y);
            if (dist > 300) {
                validPosition = true;

                for (const auto& building : buildings) {
                    if (building->isInside(pos)) {
                        validPosition = false;
                        break;
                    }
                }
            }
        }

        ZombieType type;

        if (timeOfDay == NIGHT) {
            std::uniform_int_distribution<int> nightTypeDist(0, 10);
            int roll = nightTypeDist(rng);

            if (roll < 4) type = NORMAL;
            else if (roll < 7) type = RUNNER;
            else if (roll < 9) type = TANK;
            else type = EXPLODER;
        } else {
            type = static_cast<ZombieType>(typeDist(rng));
        }

        zombies.push_back(std::make_unique<Zombie>(pos, zombieTextures[type], type));
        std::cout << "Zombie spawned - Type: " << type << " at ("
                  << pos.x << ", " << pos.y << "), Total zombies: "
                  << zombies.size() << std::endl;
    }

    void spawnItem() {
        std::uniform_real_distribution<float> xPosDist(TILE_SIZE, MAP_WIDTH - TILE_SIZE * 2);
        std::uniform_real_distribution<float> yPosDist(TILE_SIZE, MAP_HEIGHT - TILE_SIZE * 2);
        std::uniform_int_distribution<int> itemTypeDist(0, 7);
        std::uniform_int_distribution<int> valueDist(1, 5);

        ItemType type = static_cast<ItemType>(itemTypeDist(rng));

        if (timeOfDay == NIGHT) {
            std::uniform_int_distribution<int> nightTypeDist(0, 10);
            int roll = nightTypeDist(rng);

            if (roll < 3) {
                type = static_cast<ItemType>(itemTypeDist(rng));
            } else if (roll < 6) {
                std::uniform_int_distribution<int> valuableTypeDist(0, 2);
                int valuableType = valuableTypeDist(rng);
                if (valuableType == 0) type = WEAPON;
                else if (valuableType == 1) type = ARMOR;
                else type = AMMO;
            } else {
                std::uniform_int_distribution<int> veryValuableTypeDist(0, 2);
                int veryValuableType = veryValuableTypeDist(rng);
                if (veryValuableType == 0) type = WEAPON;
                else if (veryValuableType == 1) type = ARMOR;
                else type = HEALTH;
            }
        }

        int value = valueDist(rng);
        if (timeOfDay == NIGHT) value = value * 2;

        std::string name;
        switch (type) {
            case WEAPON: name = "Weapon Upgrade"; break;
            case ARMOR: name = "Armor Piece"; break;
            case HEALTH: name = "Health Pack"; break;
            case RESOURCE_WOOD: name = "Wood"; break;
            case RESOURCE_METAL: name = "Metal"; break;
            case RESOURCE_FOOD: name = "Food"; break;
            case RESOURCE_CLOTH: name = "Cloth"; break;
            case AMMO: name = "Ammo"; break;
        }

        Vector2D pos;
        bool validPosition = false;

        while (!validPosition) {
            pos.x = xPosDist(rng);
            pos.y = yPosDist(rng);

            validPosition = true;

            if (tileMap->checkCollision(Rectangle(pos.x, pos.y, TILE_SIZE, TILE_SIZE))) {
                validPosition = false;
                continue;
            }

            for (const auto& building : buildings) {
                if (building->isInside(pos)) {
                    validPosition = false;
                    break;
                }
            }
        }

        items.push_back(std::make_unique<Item>(pos, itemTextures[type], type, value, name));
    }

    void handleEvents() {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                    handleKeyDown(event.key.keysym.sym);
                    break;

                default:
                    break;
            }
        }

        if (gameState == GAMEPLAY) {
            const Uint8* keystates = SDL_GetKeyboardState(nullptr);
            player->handleInput(keystates);
        }
    }

    void handleKeyDown(SDL_Keycode key) {
        switch (gameState) {
            case MAIN_MENU:
                handleMainMenuKeys(key);
                break;
            case GAMEPLAY:
                handleGameplayKeys(key);
                break;
            case INVENTORY:
                handleInventoryKeys(key);
                break;
            case CRAFTING:
                handleCraftingKeys(key);
                break;
            case PAUSE:
                handlePauseKeys(key);
                break;
            case GAME_OVER:
                handleGameOverKeys(key);
                break;
        }
    }

    void handleMainMenuKeys(SDL_Keycode key) {
        switch (key) {
            case SDLK_RETURN:
                setupGame();
                gameState = GAMEPLAY;
                break;
            case SDLK_ESCAPE:
                running = false;
                break;
        }
    }

    void handleGameplayKeys(SDL_Keycode key) {
        switch (key) {
            case SDLK_ESCAPE:
                gameState = PAUSE;
                break;
            case SDLK_i:
                gameState = INVENTORY;
                break;
            case SDLK_c:
                gameState = CRAFTING;
                break;
            case SDLK_e:
                handleInteraction();
                break;
            case SDLK_h:
                if (player->getIsInside()) {
                    for (auto& building : buildings) {
                        if (building->isInside(player->getPosition())) {
                            player->setHomeBase(building.get());
                            building->setHomeBase(true);
                            break;
                        }
                    }
                }
                break;
        }
    }

    void handleInventoryKeys(SDL_Keycode key) {
        switch (key) {
            case SDLK_i:
            case SDLK_ESCAPE:
                gameState = GAMEPLAY;
                break;
            case SDLK_t:
                if (player->getHomeBase() && player->getIsInside()) {
                    transferItems();
                }
                break;
        }
    }

    void handleCraftingKeys(SDL_Keycode key) {
        switch (key) {
            case SDLK_c:
            case SDLK_ESCAPE:
                gameState = GAMEPLAY;
                break;
            case SDLK_1:
                craftWeaponUpgrade();
                break;
            case SDLK_2:
                craftArmorUpgrade();
                break;
            case SDLK_3:
                craftHealthPack();
                break;
            case SDLK_4:
                craftAmmo();
                break;
        }
    }

    void handlePauseKeys(SDL_Keycode key) {
        switch (key) {
            case SDLK_p:
                gameState = GAMEPLAY;
                break;
            case SDLK_ESCAPE:
                running = false;
                break;
        }
    }

    void handleGameOverKeys(SDL_Keycode key) {
        switch (key) {
            case SDLK_r:
                setupGame();
                break;
            case SDLK_ESCAPE:
                running = false;
                break;
        }
    }

    void handleInteraction() {
        bool enteredBuilding = false;

        for (auto& building : buildings) {
            if (building->isAtEntrance(player->getPosition())) {
                player->setIsInside(!player->getIsInside());
                enteredBuilding = true;
                std::cout << "Player " << (player->getIsInside() ? "entered" : "exited")
                          << " building at (" << building->getPosition().x
                          << ", " << building->getPosition().y << ")" << std::endl;
                break;
            }
        }

        if (!enteredBuilding && player->getIsInside()) {
            bool stillInside = false;
            for (auto& building : buildings) {
                if (building->isInside(player->getPosition())) {
                    stillInside = true;
                    break;
                }
            }

            if (!stillInside) {
                player->setIsInside(false);
            }
        }

        if (!player->getIsInside()) {
            for (auto it = items.begin(); it != items.end(); ) {
                if ((*it)->checkCollision(*player)) {
                    bool added = player->getInventory().addItem(**it);
                    std::cout << "Item pickup attempted: " << (*it)->getName()
                              << " - Success: " << (added ? "Yes" : "No") << std::endl;
                    if (added) {
                        it = items.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        } else {
            for (auto& building : buildings) {
                if (building->isInside(player->getPosition())) {
                    auto& buildingItems = building->getItems();
                    if (!buildingItems.empty()) {
                        Item* item = buildingItems.front();
                        bool added = player->getInventory().addItem(*item);
                        if (added) {
                            buildingItems.erase(buildingItems.begin());
                            delete item;
                        }
                    }
                    break;
                }
            }
        }
    }

    void transferItems() {
        Building* homeBase = player->getHomeBase();
        if (!homeBase || !player->getIsInside()) return;

        Inventory& playerInventory = player->getInventory();
        Inventory& storage = homeBase->getStorage();

        playerInventory.transferTo(storage, RESOURCE_WOOD, 1000);
        playerInventory.transferTo(storage, RESOURCE_METAL, 1000);
        playerInventory.transferTo(storage, RESOURCE_FOOD, 1000);
        playerInventory.transferTo(storage, RESOURCE_CLOTH, 1000);

        storage.transferTo(playerInventory, WEAPON, 1000);
        storage.transferTo(playerInventory, ARMOR, 1000);
        storage.transferTo(playerInventory, HEALTH, 1000);
        storage.transferTo(playerInventory, AMMO, 1000);
    }

    void craftWeaponUpgrade() {
        Inventory& inv = player->getInventory();

        if (inv.getItemCount(RESOURCE_METAL) >= 5 && inv.getItemCount(RESOURCE_WOOD) >= 3) {
            inv.useItem(RESOURCE_METAL, 5);
            inv.useItem(RESOURCE_WOOD, 3);
            player->upgradeWeapon(10);
        }
    }

    void craftArmorUpgrade() {
        Inventory& inv = player->getInventory();

        if (inv.getItemCount(RESOURCE_CLOTH) >= 8 && inv.getItemCount(RESOURCE_METAL) >= 2) {
            inv.useItem(RESOURCE_CLOTH, 8);
            inv.useItem(RESOURCE_METAL, 2);
            player->upgradeArmor(10);
        }
    }

    void craftHealthPack() {
        Inventory& inv = player->getInventory();

        if (inv.getItemCount(RESOURCE_FOOD) >= 5 && inv.getItemCount(RESOURCE_CLOTH) >= 2) {
            inv.useItem(RESOURCE_FOOD, 5);
            inv.useItem(RESOURCE_CLOTH, 2);
            inv.addItem(Item(Vector2D(0, 0), itemTextures[HEALTH], HEALTH, 1, "Health Pack"));
        }
    }

    void craftAmmo() {
        Inventory& inv = player->getInventory();

        if (inv.getItemCount(RESOURCE_METAL) >= 3 && inv.getItemCount(RESOURCE_WOOD) >= 2) {
            inv.useItem(RESOURCE_METAL, 3);
            inv.useItem(RESOURCE_WOOD, 2);
            inv.addItem(Item(Vector2D(0, 0), itemTextures[AMMO], AMMO, 10, "Ammo"));
        }
    }

    void update() {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 deltaTime = currentTime - lastFrameTime;

        std::cout << "Game update - Time: " << gameTime
                  << "ms, Delta: " << deltaTime << "ms" << std::endl;

        lastFrameTime = currentTime;
	
	if (gameState != GAMEPLAY) {
    printf("gamestate=%d, gameplay=%d\n", gameState, GAMEPLAY);
    return;
}

	printf("about to run gametime +=deltatime");

        gameTime += deltaTime;

        if (currentTime - lastTimeUpdate > 1000) {
            Uint32 cycleTime = gameTime % DAY_NIGHT_CYCLE_DURATION;
            TimeOfDay newTimeOfDay = (cycleTime < DAY_NIGHT_CYCLE_DURATION * DAY_RATIO) ? DAY : NIGHT;

            if (newTimeOfDay != timeOfDay) {
                timeOfDay = newTimeOfDay;

                if (timeOfDay == NIGHT) {
                    int nightSpawnCount = 10 + rand() % 10;
                    for (int i = 0; i < nightSpawnCount; ++i) {
                        spawnZombie();
                    }

                    int nightItemCount = 5 + rand() % 5;
                    for (int i = 0; i < nightItemCount; ++i) {
                        spawnItem();
                    }
                }
            }

            lastTimeUpdate = currentTime;
        }
        printf("before player update");
        player->update();
	printf("After player update");
        bool inBuilding = false;

        for (auto& building : buildings) {
            if (player->getIsInside()) {
                if (building->isInside(player->getPosition())) {
                    inBuilding = true;
                }
            } else {
                if (player->checkCollision(*building)) {
                    Vector2D pushDirection = (player->getPosition() - building->getPosition()).normalize();
                    player->setPosition(player->getPosition() + pushDirection * 5.0f);
                }
            }
        }

        if (player->getIsInside() && !inBuilding) {
            player->setIsInside(false);
        }

        for (auto& zombie : zombies) {
            if (zombie->isActive()) {
                bool zombieInBuilding = false;
                for (auto& building : buildings) {
                    if (building->isInside(zombie->getPosition())) {
                        zombieInBuilding = true;
                        break;
                    }
                }

                if (!zombieInBuilding) {
                    zombie->update(*player);

                    for (auto& building : buildings) {
                        if (zombie->checkCollision(*building)) {
                            Vector2D pushDirection = (zombie->getPosition() - building->getPosition()).normalize();
                            zombie->setPosition(zombie->getPosition() + pushDirection * 3.0f);
                        }
                    }
                }
            }
        }

        for (auto& zombie : zombies) {
            if (zombie->isActive() && !player->getIsInside()) {
                if (zombie->checkCollision(*player)) {
                    if (player->getInventory().getItemCount(AMMO) > 0) {
                        zombie->takeDamage(player->getWeaponPower() * 2);
                        player->getInventory().useItem(AMMO, 1);
                    } else {
                        zombie->takeDamage(player->getWeaponPower());
                    }
                }
            }
        }

        zombies.erase(
            std::remove_if(zombies.begin(), zombies.end(),
                [](const std::unique_ptr<Zombie>& z) { return !z->isActive(); }),
            zombies.end()
        );

        if (currentTime - lastZombieSpawn > zombieSpawnInterval) {
            int spawnCount = (timeOfDay == DAY) ? 1 : 3;

            for (int i = 0; i < spawnCount; ++i) {
                spawnZombie();
            }

            lastZombieSpawn = currentTime;
        }

        if (rand() % 100 < 1) {
            spawnItem();
        }

        if (player->getHealth() < player->getMaxHealth() / 2 && player->getInventory().getItemCount(HEALTH) > 0) {
            player->getInventory().useItem(HEALTH, 1);
            player->heal(25);
        }

        if (player->getHealth() <= 0) {
            gameState = GAME_OVER;
            std::cout << "Game Over - Player health depleted" << std::endl;
        }

        camera.update(player->getPosition());
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

       // if (gameState == MAIN_MENU) {
        //    uiManager->render();
        //    SDL_RenderPresent(renderer);
        //    return;
       // }

        SDL_Rect viewport = camera.getViewport();

        tileMap->render(renderer, camera);

        for (auto& building : buildings) {
            if (camera.isVisible(building->getCollider())) {
                building->render(renderer, camera);

                if (building->isHomeBase()) {
                    Vector2D screenPos = camera.worldToScreen(building->getPosition());
                    SDL_Rect outlineRect = {
                        static_cast<int>(screenPos.x) - 2,
                        static_cast<int>(screenPos.y) - 2,
                        static_cast<int>(building->getCollider().w) + 4,
                        static_cast<int>(building->getCollider().h) + 4
                    };

                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                    SDL_RenderDrawRect(renderer, &outlineRect);
                }
            }
        }

        for (auto& item : items) {
            if (camera.isVisible(item->getCollider())) {
                item->render(renderer, camera);
            }
        }

        for (auto& zombie : zombies) {
            if (zombie->isActive() && camera.isVisible(zombie->getCollider())) {
                zombie->render(renderer, camera);
            }
        }

        if (!player->getIsInside()) {
            player->render(renderer, camera);
        }

        // uiManager->render();

        SDL_RenderPresent(renderer);
    }

    void clean() {
        if (playerTexture) SDL_DestroyTexture(playerTexture);

        for (int i = 0; i < 4; ++i) {
            if (zombieTextures[i]) SDL_DestroyTexture(zombieTextures[i]);
        }

        for (int i = 0; i < 3; ++i) {
            if (buildingTextures[i]) SDL_DestroyTexture(buildingTextures[i]);
        }

        for (int i = 0; i < 8; ++i) {
            if (itemTextures[i]) SDL_DestroyTexture(itemTextures[i]);
        }

        if (tilesetTexture) SDL_DestroyTexture(tilesetTexture);

        for (int i = 0; i < 2; ++i) {
            if (uiTextures[i]) SDL_DestroyTexture(uiTextures[i]);
        }

        if (font) TTF_CloseFont(font);

        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }

    bool isRunning() const {
        return running;
    }

    void capFrameRate(Uint32 frameStart) {
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }
};

// Main function
int main(int argc, char* argv[]) {
    Game game;
    printf("About to do game. init");
    //game.gameState=GAMEPLAY;
    if (!game.init("Zombie Survival", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, false)) {
        std::cerr << "Game initialization failed!" << std::endl;
        return 1;
    }
    printf("Game is about to be running");
    while (game.isRunning()) {
        Uint32 frameStart = SDL_GetTicks();

        game.handleEvents();
	printf("HandleEvents");
        game.update();
	printf("About to run game render");
        game.render();

        game.capFrameRate(frameStart);
    }

    return 0;
}
