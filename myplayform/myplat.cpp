#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <memory>
#include <cmath>
#include <string>
#include <random>
#include <chrono>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GRAVITY = 1;
const int PLAYER_SPEED = 5;
const int JUMP_FORCE = 15;
const int CLIMB_SPEED = 3;
const float SEARCH_TIME = 5.0f; // Seconds to search
const int MAX_ARMOR_PIECES = 5;

// Enums
enum class EntityType {
    PLAYER,
    ENEMY,
    PLATFORM,
    LADDER,
    SEARCHABLE,
    PROJECTILE
};

enum class ItemType {
    NONE,
    AMMO,
    ARMOR,
    HEALTH
};

enum class EnemyType {
    BASIC,    // 2 HP
    MEDIUM,   // 4 HP
    ARMORED   // 8 HP
};

// Base entity class
class Entity {
public:
    Entity(float x, float y, int w, int h, EntityType type) 
        : x(x), y(y), width(w), height(h), type(type), velocityX(0), velocityY(0) {}
    
    virtual ~Entity() {}
    
    virtual void update() {}
    virtual void render(SDL_Renderer* renderer) {}
    
    SDL_Rect getRect() const {
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
    bool collidesWith(const Entity& other) const {
        SDL_Rect rect1 = getRect();
        SDL_Rect rect2 = other.getRect();
        return SDL_HasIntersection(&rect1, &rect2);
    }
    
    EntityType getType() const { return type; }
    
    float x, y;
    int width, height;
    float velocityX, velocityY;
    
protected:
    EntityType type;
};

// Platform class
class Platform : public Entity {
public:
    Platform(float x, float y, int w, int h) : Entity(x, y, w, h, EntityType::PLATFORM) {}
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = getRect();
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
};

// Ladder class
class Ladder : public Entity {
public:
    Ladder(float x, float y, int w, int h) : Entity(x, y, w, h, EntityType::LADDER) {}
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = getRect();
        SDL_SetRenderDrawColor(renderer, 150, 75, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw rungs
        SDL_SetRenderDrawColor(renderer, 200, 100, 0, 255);
        for (int i = 10; i < height; i += 20) {
            SDL_RenderDrawLine(renderer, x, y + i, x + width, y + i);
        }
    }
};

// Searchable item class
class SearchableItem : public Entity {
public:
    SearchableItem(float x, float y, ItemType itemType) 
        : Entity(x, y, 30, 30, EntityType::SEARCHABLE), 
          itemType(itemType), 
          searched(false), 
          searchProgress(0.0f) {}
    
    void update() override {
        // Nothing specific to update
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = getRect();
        
        // Draw the searchable object
        if (!searched) {
            // Color based on item type
            switch (itemType) {
                case ItemType::AMMO:
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
                    break;
                case ItemType::ARMOR:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue
                    break;
                case ItemType::HEALTH:
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
                    break;
                default:
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Gray
            }
            
            SDL_RenderFillRect(renderer, &rect);
            
            // Draw search progress circle if being searched
            if (searchProgress > 0.0f && searchProgress < SEARCH_TIME) {
                int centerX = rect.x + rect.w / 2;
                int centerY = rect.y - 20;
                int radius = 10;
                
                // Draw complete circle outline
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                for (int i = 0; i < 360; i++) {
                    float angle = i * M_PI / 180.0f;
                    int px = centerX + static_cast<int>(radius * cos(angle));
                    int py = centerY + static_cast<int>(radius * sin(angle));
                    SDL_RenderDrawPoint(renderer, px, py);
                }
                
                // Draw filled portion based on search progress
                float progressRatio = searchProgress / SEARCH_TIME;
                int endAngle = static_cast<int>(360.0f * progressRatio);
                
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                for (int i = 0; i < endAngle; i++) {
                    float angle = i * M_PI / 180.0f;
                    for (int r = 0; r < radius; r++) {
                        int px = centerX + static_cast<int>(r * cos(angle));
                        int py = centerY + static_cast<int>(r * sin(angle));
                        SDL_RenderDrawPoint(renderer, px, py);
                    }
                }
            }
        }
    }
    
    void search(float deltaTime) {
        if (!searched) {
            searchProgress += deltaTime;
            if (searchProgress >= SEARCH_TIME) {
                searched = true;
            }
        }
    }
    
    void resetSearch() {
        if (!searched) {
            searchProgress = 0.0f;
        }
    }
    
    bool isSearched() const { return searched; }
    ItemType getItemType() const { return itemType; }
    
private:
    ItemType itemType;
    bool searched;
    float searchProgress;
};

// Projectile class
class Projectile : public Entity {
public:
    Projectile(float x, float y, float dirX, float dirY, bool fromPlayer) 
        : Entity(x, y, 10, 10, EntityType::PROJECTILE),
          fromPlayer(fromPlayer),
          lifeTime(2.0f) {
        
        const float PROJECTILE_SPEED = 8.0f;
        velocityX = dirX * PROJECTILE_SPEED;
        velocityY = dirY * PROJECTILE_SPEED;
    }
    
    void update() override {
        x += velocityX;
        y += velocityY;
        
        lifeTime -= 0.016f; // Assuming 60 FPS, roughly 16ms per frame
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = getRect();
        
        if (fromPlayer) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Cyan for player projectiles
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for enemy projectiles
        }
        
        SDL_RenderFillRect(renderer, &rect);
    }
    
    bool isFromPlayer() const { return fromPlayer; }
    bool isDead() const { return lifeTime <= 0.0f; }
    
private:
    bool fromPlayer;
    float lifeTime;
};

// Enemy class
class Enemy : public Entity {
public:
    Enemy(float x, float y, EnemyType enemyType) 
        : Entity(x, y, 30, 50, EntityType::ENEMY),
          enemyType(enemyType),
          direction(1),
          shootCooldown(0.0f),
          hitPoints(getInitialHP(enemyType)) {}
    
    void update() override {
        // Basic movement - patrol left and right
        x += direction * 2;
        
        // Update shooting cooldown
        if (shootCooldown > 0.0f) {
            shootCooldown -= 0.016f; // Assuming 60 FPS
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = getRect();
        
        // Different colors for different enemy types
        switch (enemyType) {
            case EnemyType::BASIC:
                SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                break;
            case EnemyType::MEDIUM:
                SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
                break;
            case EnemyType::ARMORED:
                SDL_SetRenderDrawColor(renderer, 150, 0, 0, 255);
                break;
        }
        
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw HP bar
        int maxHP = getInitialHP(enemyType);
        float hpRatio = static_cast<float>(hitPoints) / maxHP;
        
        SDL_Rect hpBar = {
            static_cast<int>(x), 
            static_cast<int>(y - 10), 
            static_cast<int>(width * hpRatio), 
            5
        };
        
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &hpBar);
    }
    
    bool canShoot() const {
        return shootCooldown <= 0.0f;
    }
    
    void resetShootCooldown() {
        // Different enemy types have different fire rates
        switch (enemyType) {
            case EnemyType::BASIC:
                shootCooldown = 3.0f;
                break;
            case EnemyType::MEDIUM:
                shootCooldown = 2.0f;
                break;
            case EnemyType::ARMORED:
                shootCooldown = 1.5f;
                break;
        }
    }
    
    void reverseDirection() {
        direction *= -1;
    }
    
    void takeDamage(int damage) {
        hitPoints -= damage;
    }
    
    bool isDead() const {
        return hitPoints <= 0;
    }
    
private:
    EnemyType enemyType;
    int direction;
    float shootCooldown;
    int hitPoints;
    
    int getInitialHP(EnemyType type) const {
        switch (type) {
            case EnemyType::BASIC: return 2;
            case EnemyType::MEDIUM: return 4;
            case EnemyType::ARMORED: return 8;
            default: return 1;
        }
    }
};

// Player class
class Player : public Entity {
public:
    Player(float x, float y) 
        : Entity(x, y, 30, 50, EntityType::PLAYER),
          health(100),
          maxHealth(100),
          ammo(30),
          armorPieces(0),
          armorLevel(0),
          isOnGround(false),
          isOnLadder(false),
          isFacingRight(true),
          isSearching(false),
          searchingItem(nullptr),
          shootCooldown(0.0f) {}
    
    void update(const std::vector<std::shared_ptr<Entity>>& entities, float deltaTime) {
        // Apply gravity if not on ladder
        if (!isOnLadder) {
            velocityY += GRAVITY;
        }
        
        // Apply velocity
        x += velocityX;
        y += velocityY;
        
        // Reset flags
        isOnGround = false;
        isOnLadder = false;
        
        // Handle collisions with platforms and ladders
        for (const auto& entity : entities) {
            if (entity->getType() == EntityType::PLATFORM && collidesWith(*entity)) {
                // If falling onto platform
                if (velocityY > 0 && y + height - velocityY <= entity->y) {
                    y = entity->y - height;
                    velocityY = 0;
                    isOnGround = true;
                }
                // Side or bottom collision with platform
                else if (x + width > entity->x && x < entity->x + entity->width) {
                    if (velocityX > 0) {
                        x = entity->x - width;
                    } else if (velocityX < 0) {
                        x = entity->x + entity->width;
                    }
                    velocityX = 0;
                }
            }
            else if (entity->getType() == EntityType::LADDER && collidesWith(*entity)) {
                isOnLadder = true;
                // Slow down vertical velocity on ladder
                velocityY *= 0.5f;
            }
        }
        
        // Keep player within screen bounds
        if (x < 0) x = 0;
        if (x + width > SCREEN_WIDTH) x = SCREEN_WIDTH - width;
        if (y < 0) y = 0;
        if (y + height > SCREEN_HEIGHT) y = SCREEN_HEIGHT - height;
        
        // Update shooting cooldown
        if (shootCooldown > 0.0f) {
            shootCooldown -= deltaTime;
        }
        
        // Handle searching
        if (isSearching && searchingItem) {
            searchingItem->search(deltaTime);
            if (searchingItem->isSearched()) {
                collectItem(searchingItem->getItemType());
                isSearching = false;
                searchingItem = nullptr;
            }
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = getRect();
        
        // Draw player with color based on armor level
        switch (armorLevel) {
            case 0:
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White - no armor
                break;
            case 1:
                SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255); // Light blue - light armor
                break;
            case 2:
                SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255); // Medium blue - medium armor
                break;
            case 3:
                SDL_SetRenderDrawColor(renderer, 50, 50, 255, 255); // Deep blue - heavy armor
                break;
        }
        
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw direction indicator
        int indicatorX = isFacingRight ? rect.x + rect.w - 5 : rect.x;
        SDL_Rect indicator = {indicatorX, rect.y + 15, 5, 5};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &indicator);
        
        // Draw health bar
        float healthRatio = static_cast<float>(health) / maxHealth;
        SDL_Rect healthBar = {
            rect.x, 
            rect.y - 15, 
            static_cast<int>(rect.w * healthRatio), 
            5
        };
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
        
        // Draw armor pieces counter
        SDL_Rect armorPieceRect = {rect.x, rect.y - 25, 0, 5};
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        
        for (int i = 0; i < armorPieces; i++) {
            armorPieceRect.x = rect.x + (i * 7);
            armorPieceRect.w = 5;
            SDL_RenderFillRect(renderer, &armorPieceRect);
        }
    }
    
    void moveLeft() {
        velocityX = -PLAYER_SPEED;
        isFacingRight = false;
    }
    
    void moveRight() {
        velocityX = PLAYER_SPEED;
        isFacingRight = true;
    }
    
    void stopMoving() {
        velocityX = 0;
    }
    
    void jump() {
        if (isOnGround) {
            velocityY = -JUMP_FORCE;
        }
    }
    
    void climbUp() {
        if (isOnLadder) {
            velocityY = -CLIMB_SPEED;
        }
    }
    
    void climbDown() {
        if (isOnLadder) {
            velocityY = CLIMB_SPEED;
        }
    }
    
    void stopClimbing() {
        if (isOnLadder) {
            velocityY = 0;
        }
    }
    
    std::shared_ptr<Projectile> shoot() {
        if (shootCooldown <= 0.0f && ammo > 0) {
            float dirX = isFacingRight ? 1.0f : -1.0f;
            float startX = isFacingRight ? x + width : x;
            
            ammo--;
            shootCooldown = 0.25f; // 4 shots per second max
            
            return std::make_shared<Projectile>(startX, y + height/2 - 5, dirX, 0, true);
        }
        return nullptr;
    }
    
    bool startSearching(const std::vector<std::shared_ptr<Entity>>& entities) {
        // Stop searching current item, if any
        stopSearching();
        
        // Find searchable items player is touching
        for (const auto& entity : entities) {
            if (entity->getType() == EntityType::SEARCHABLE && collidesWith(*entity)) {
                auto searchable = std::dynamic_pointer_cast<SearchableItem>(entity);
                if (searchable && !searchable->isSearched()) {
                    isSearching = true;
                    searchingItem = searchable;
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void stopSearching() {
        if (isSearching && searchingItem) {
            searchingItem->resetSearch();
            isSearching = false;
            searchingItem = nullptr;
        }
    }
    
    void takeDamage(int damage) {
        // Reduce damage based on armor level
        float damageReduction = armorLevel * 0.25f; // 0%, 25%, 50%, 75% reduction
        int actualDamage = static_cast<int>(damage * (1.0f - damageReduction));
        
        health -= actualDamage;
        if (health < 0) health = 0;
    }
    
    void collectItem(ItemType type) {
        switch(type) {
            case ItemType::AMMO:
                ammo += 20;
                break;
            case ItemType::ARMOR:
                armorPieces++;
                if (armorPieces >= MAX_ARMOR_PIECES) {
                    armorPieces = 0;
                    armorLevel = std::min(armorLevel + 1, 3); // Max level 3
                }
                break;
            case ItemType::HEALTH:
                health = std::min(health + 25, maxHealth);
                break;
            default:
                break;
        }
    }
    
    bool isDead() const {
        return health <= 0;
    }
    
    int getHealth() const { return health; }
    int getAmmo() const { return ammo; }
    int getArmorPieces() const { return armorPieces; }
    int getArmorLevel() const { return armorLevel; }
    
private:
    int health;
    int maxHealth;
    int ammo;
    int armorPieces;
    int armorLevel;
    bool isOnGround;
    bool isOnLadder;
    bool isFacingRight;
    bool isSearching;
    std::shared_ptr<SearchableItem> searchingItem;
    float shootCooldown;
};

// Game class
class Game {
public:
    Game() : running(false), player(nullptr) {}
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            return false;
        }
        
        window = SDL_CreateWindow("Platform Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                 SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            return false;
        }
        
        running = true;
        lastUpdateTime = std::chrono::high_resolution_clock::now();
        
        setupLevel();
        
        return true;
    }
    
    void setupLevel() {
        entities.clear();
        projectiles.clear();
        
        // Create player
        player = std::make_shared<Player>(100, 100);
        
        // Create platforms
        entities.push_back(std::make_shared<Platform>(0, 550, 800, 50));    // Ground
        entities.push_back(std::make_shared<Platform>(100, 450, 200, 20));  // Platform 1
        entities.push_back(std::make_shared<Platform>(400, 350, 200, 20));  // Platform 2
        entities.push_back(std::make_shared<Platform>(200, 250, 200, 20));  // Platform 3
        entities.push_back(std::make_shared<Platform>(500, 150, 200, 20));  // Platform 4
        
        // Create ladders
        entities.push_back(std::make_shared<Ladder>(300, 450, 20, 100));    // Ladder 1
        entities.push_back(std::make_shared<Ladder>(500, 350, 20, 100));    // Ladder 2
        entities.push_back(std::make_shared<Ladder>(300, 250, 20, 100));    // Ladder 3
        entities.push_back(std::make_shared<Ladder>(600, 150, 20, 100));    // Ladder 4
        
        // Create searchable items
        entities.push_back(std::make_shared<SearchableItem>(150, 420, ItemType::AMMO));
        entities.push_back(std::make_shared<SearchableItem>(250, 420, ItemType::ARMOR));
        entities.push_back(std::make_shared<SearchableItem>(450, 320, ItemType::HEALTH));
        entities.push_back(std::make_shared<SearchableItem>(550, 320, ItemType::ARMOR));
        entities.push_back(std::make_shared<SearchableItem>(250, 220, ItemType::AMMO));
        entities.push_back(std::make_shared<SearchableItem>(350, 220, ItemType::ARMOR));
        entities.push_back(std::make_shared<SearchableItem>(550, 120, ItemType::HEALTH));
        entities.push_back(std::make_shared<SearchableItem>(650, 120, ItemType::ARMOR));
        
        // Create enemies
        entities.push_back(std::make_shared<Enemy>(200, 400, EnemyType::BASIC));
        entities.push_back(std::make_shared<Enemy>(500, 300, EnemyType::MEDIUM));
        entities.push_back(std::make_shared<Enemy>(300, 200, EnemyType::BASIC));
        entities.push_back(std::make_shared<Enemy>(600, 100, EnemyType::ARMORED));
    }
    
    void run() {
        SDL_Event event;
        
        while (running) {
            // Calculate delta time for smooth movement
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
            lastUpdateTime = currentTime;
            
            // Process events
            while (SDL_PollEvent(&event)) {
                handleEvent(event);
            }
            
            // Update game state
            update(deltaTime);
            
            // Render
            render();
            
            // Cap frame rate at roughly 60 FPS
            SDL_Delay(16);
        }
    }
    
    void handleEvent(const SDL_Event& event) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    player->moveLeft();
                    break;
                case SDLK_RIGHT:
                    player->moveRight();
                    break;
                case SDLK_UP:
                    if (player->isOnLadder) {
                        player->climbUp();
                    } else {
                        player->jump();
                    }
                    break;
                case SDLK_DOWN:
                    player->climbDown();
                    break;
                case SDLK_SPACE:
                    player->startSearching(entities);
                    break;
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    auto projectile = player->shoot();
                    if (projectile) {
                        projectiles.push_back(projectile);
                    }
                    break;
            }
        }
        else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                case SDLK_RIGHT:
                    player->stopMoving();
                    break;
                case SDLK_UP:
                case SDLK_DOWN:
                    player->stopClimbing();
                    break;
                case SDLK_SPACE:
                    player->stopSearching();
                    break;
            }
        }
    }
    
    void update(float deltaTime) {
        if (!player || player->isDead()) {
            return;
        }
        
        // Update player
        player->update(entities, deltaTime);
        
        // Update enemies
        for (auto& entity : entities) {
            if (entity->getType() == EntityType::ENEMY) {
                auto enemy = std::dynamic_pointer_cast<Enemy>(entity);
                
                enemy->update();
                
                // Check if enemy hits platform edge or wall
                bool shouldReverse = false;
                if (enemy->x <= 0 || enemy->x + enemy->width >= SCREEN_WIDTH) {
                    shouldReverse = true;
                }
                
                for (auto& platform : entities) {
                    if (platform->getType() == EntityType::PLATFORM) {
                        // Check if enemy is about to walk off platform
                        if (enemy->velocityX > 0 &&
                            std::abs((enemy->x + enemy->width) - platform->x) < 5 &&
                            enemy->y + enemy->height == platform->y) {
                            shouldReverse = true;
                        }
                        else if (enemy->velocityX < 0 &&
                                std::abs(enemy->x - (platform->x + platform->width)) < 5 &&
                                enemy->y + enemy->height == platform->y) {
                            shouldReverse = true;
                        }
                    }
                }
                
                if (shouldReverse) {
                    enemy->reverseDirection();
                }
                
                // Enemy shooting
                if (enemy->canShoot()) {
                    // Simple line of sight check
                    bool canSeePlayer = std::abs(player->y - enemy->y) < 30;
                    
                    if (canSeePlayer) {
                        float dirX = (player->x > enemy->x) ? 1.0f : -1.0f;
                        float startX = (dirX > 0) ? enemy->x + enemy->width : enemy->x;
                        
                        projectiles.push_back(std::make_shared<Projectile>(
                            startX, enemy->y + enemy->height/2 - 5, dirX, 0, false));
                        
                        enemy->resetShootCooldown();
                    }
                }
            }
            else {
                entity->update();
            }
        }
        
        // Update projectiles
        for (auto it = projectiles.begin(); it != projectiles.end();) {
            auto& projectile = *it;
            projectile->update();
            
            bool shouldRemove = false;
            
            // Check if projectile is out of bounds
            if (projectile->x < 0 || projectile->x > SCREEN_WIDTH ||
                projectile->y < 0 || projectile->y > SCREEN_HEIGHT ||
                projectile->isDead()) {
                shouldRemove = true;
            }
            
            // Check collisions with platforms
            for (auto& entity : entities) {
                if (entity->getType() == EntityType::PLATFORM && 
                    projectile->collidesWith(*entity)) {
                    shouldRemove = true;
                    break;
                }
            }
            
            // Check collisions with enemies (for player projectiles)
            if (projectile->isFromPlayer()) {
                for (auto& entity : entities) {
                    if (entity->getType() == EntityType::ENEMY && 
                        projectile->collidesWith(*entity)) {
                        
                        auto enemy = std::dynamic_pointer_cast<Enemy>(entity);
                        enemy->takeDamage(1);
                        
                        shouldRemove = true;
                        break;
                    }
                }
            }
            // Check collisions with player (for enemy projectiles)
            else if (!projectile->isFromPlayer() && projectile->collidesWith(*player)) {
                player->takeDamage(10);
                shouldRemove = true;
            }
            
            if (shouldRemove) {
                it = projectiles.erase(it);
            } else {
                ++it;
            }
        }
        
        // Remove dead enemies
        for (auto it = entities.begin(); it != entities.end();) {
            if ((*it)->getType() == EntityType::ENEMY) {
                auto enemy = std::dynamic_pointer_cast<Enemy>(*it);
                if (enemy->isDead()) {
                    it = entities.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
    
    void render() {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(
