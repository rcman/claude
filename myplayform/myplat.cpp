#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <memory>
#include <cmath>
#include <string>
#include <map>
#include <functional>
#include <iostream>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float GRAVITY = 0.5f;
const float JUMP_FORCE = -12.0f;
const int PLAYER_SPEED = 5;
const int SEARCH_TIME = 5000; // 5 seconds in milliseconds
const int ARMOR_PIECES_FOR_UPGRADE = 5;

// Forward declarations
class GameObject;
class Player;
class Enemy;
class Platform;
class Ladder;
class SearchableItem;

// Game state
enum class GameState {
    RUNNING,
    PAUSED,
    GAME_OVER
};

// Armor types
enum class ArmorType {
    HEAD,
    CHEST,
    LEGS,
    ARMS,
    FEET
};

// Item types
enum class ItemType {
    AMMO,
    ARMOR,
    HEALTH
};

// Simple Vector2 class for positions and velocities
struct Vector2 {
    float x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
    
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }
    
    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }
    
    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
};

// Base GameObject class
class GameObject {
public:
    Vector2 position;
    Vector2 velocity;
    int width, height;
    bool active;
    SDL_Texture* texture;
    
    GameObject(float x, float y, int w, int h) : 
        position(x, y), velocity(0, 0), width(w), height(h), active(true), texture(nullptr) {}
    
    virtual ~GameObject() {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    
    virtual void Update(float deltaTime) {
        position += velocity;
    }
    
    virtual void Render(SDL_Renderer* renderer) {
        if (!active) return;
        
        SDL_Rect rect = {
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            width,
            height
        };
        
        if (texture) {
            SDL_RenderCopy(renderer, texture, nullptr, &rect);
        } else {
            // Default rendering if no texture is available
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    
    SDL_Rect GetRect() const {
        return {
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            width,
            height
        };
    }
    
    bool CheckCollision(const GameObject* other) const {
        if (!active || !other->active) return false;
        
        SDL_Rect a = GetRect();
        SDL_Rect b = other->GetRect();
        
        return SDL_HasIntersection(&a, &b);
    }
};

// Player class
class Player : public GameObject {
public:
    int health;
    int maxHealth;
    int ammo;
    std::map<ArmorType, bool> armorPieces;
    bool isOnGround;
    bool isOnLadder;
    bool isShooting;
    bool isSearching;
    Uint32 searchStartTime;
    float searchProgress;
    SearchableItem* currentSearchItem;
    int armorLevel;
    
    Player(float x, float y) : 
        GameObject(x, y, 32, 64),
        health(100),
        maxHealth(100),
        ammo(10),
        isOnGround(false),
        isOnLadder(false),
        isShooting(false),
        isSearching(false),
        searchStartTime(0),
        searchProgress(0),
        currentSearchItem(nullptr),
        armorLevel(0) {
        // Initialize armor pieces to not collected
        armorPieces[ArmorType::HEAD] = false;
        armorPieces[ArmorType::CHEST] = false;
        armorPieces[ArmorType::LEGS] = false;
        armorPieces[ArmorType::ARMS] = false;
        armorPieces[ArmorType::FEET] = false;
    }
    
    void Update(float deltaTime) override;
    
    void Render(SDL_Renderer* renderer) override {
        GameObject::Render(renderer);
        
        // Render health bar
        SDL_Rect healthBar = {
            static_cast<int>(position.x),
            static_cast<int>(position.y - 10),
            width,
            5
        };
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
        
        healthBar.w = static_cast<int>((float)health / maxHealth * width);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
        
        // Render search progress if searching
        if (isSearching && currentSearchItem) {
            const int radius = 15;
            const int centerX = static_cast<int>(currentSearchItem->position.x + currentSearchItem->width / 2);
            const int centerY = static_cast<int>(currentSearchItem->position.y + currentSearchItem->height / 2);
            
            // Draw background circle
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            for (int w = 0; w < radius * 2; w++) {
                for (int h = 0; h < radius * 2; h++) {
                    int dx = radius - w;
                    int dy = radius - h;
                    if ((dx*dx + dy*dy) <= (radius * radius)) {
                        SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
                    }
                }
            }
            
            // Draw progress arc
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            float angle = searchProgress * 2 * M_PI;
            for (float a = 0; a < angle; a += 0.01f) {
                int x = static_cast<int>(centerX + cos(a) * radius);
                int y = static_cast<int>(centerY + sin(a) * radius);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    
    void Jump() {
        if (isOnGround) {
            velocity.y = JUMP_FORCE;
            isOnGround = false;
        }
    }
    
    void Shoot();
    
    void TakeDamage(int amount) {
        // Apply armor reduction if available
        float damageReduction = 0.0f;
        switch(armorLevel) {
            case 1: damageReduction = 0.1f; break;
            case 2: damageReduction = 0.2f; break;
            case 3: damageReduction = 0.3f; break;
            case 4: damageReduction = 0.4f; break;
            case 5: damageReduction = 0.5f; break;
            default: damageReduction = 0.0f; break;
        }
        
        int actualDamage = static_cast<int>(amount * (1.0f - damageReduction));
        health -= actualDamage;
        if (health <= 0) {
            health = 0;
            active = false;
        }
    }
    
    void AddArmorPiece(ArmorType type) {
        if (!armorPieces[type]) {
            armorPieces[type] = true;
            
            // Count collected pieces
            int count = 0;
            for (const auto& piece : armorPieces) {
                if (piece.second) count++;
            }
            
            // Update armor level
            armorLevel = count / ARMOR_PIECES_FOR_UPGRADE;
        }
    }
    
    void StartSearching(SearchableItem* item) {
        if (!isSearching && item) {
            isSearching = true;
            searchStartTime = SDL_GetTicks();
            currentSearchItem = item;
            searchProgress = 0.0f;
        }
    }
    
    void StopSearching() {
        isSearching = false;
        currentSearchItem = nullptr;
        searchProgress = 0.0f;
    }
    
    bool UpdateSearching();
};

// Platform class
class Platform : public GameObject {
public:
    Platform(float x, float y, int w, int h) : GameObject(x, y, w, h) {}
    
    void Render(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brown color
        SDL_Rect rect = GetRect();
        SDL_RenderFillRect(renderer, &rect);
    }
};

// Ladder class
class Ladder : public GameObject {
public:
    Ladder(float x, float y, int w, int h) : GameObject(x, y, w, h) {}
    
    void Render(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255); // Sienna color
        SDL_Rect rect = GetRect();
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw rungs
        SDL_SetRenderDrawColor(renderer, 205, 133, 63, 255); // Peru color
        for (int y = 10; y < height; y += 15) {
            SDL_Rect rung = {
                static_cast<int>(position.x),
                static_cast<int>(position.y + y),
                width,
                3
            };
            SDL_RenderFillRect(renderer, &rung);
        }
    }
};

// Projectile class
class Projectile : public GameObject {
public:
    int damage;
    bool fromPlayer;
    
    Projectile(float x, float y, float dirX, int dmg, bool playerShot) :
        GameObject(x, y, 8, 4),
        damage(dmg),
        fromPlayer(playerShot) {
        velocity.x = dirX * 10.0f;
    }
    
    void Update(float deltaTime) override {
        GameObject::Update(deltaTime);
        
        // Deactivate if offscreen
        if (position.x < 0 || position.x > SCREEN_WIDTH) {
            active = false;
        }
    }
    
    void Render(SDL_Renderer* renderer) override {
        if (!active) return;
        
        SDL_SetRenderDrawColor(renderer, fromPlayer ? 255 : 255, fromPlayer ? 255 : 0, 0, 255);
        SDL_Rect rect = GetRect();
        SDL_RenderFillRect(renderer, &rect);
    }
};

// Enemy class
class Enemy : public GameObject {
public:
    int health;
    int maxHealth;
    int damage;
    float shootCooldown;
    float timeSinceLastShot;
    float moveDirection;
    float moveTimer;
    float playerDetectionRange;
    
    Enemy(float x, float y, int hp) : 
        GameObject(x, y, 32, 40),
        health(hp),
        maxHealth(hp),
        damage(10),
        shootCooldown(2.0f),
        timeSinceLastShot(0),
        moveDirection(1.0f),
        moveTimer(0),
        playerDetectionRange(300.0f) {}
    
    void Update(float deltaTime, Player* player, std::vector<std::shared_ptr<Projectile>>& projectiles);
    
    void Render(SDL_Renderer* renderer) override {
        if (!active) return;
        
        // Draw enemy
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect rect = GetRect();
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw health bar
        SDL_Rect healthBar = {
            static_cast<int>(position.x),
            static_cast<int>(position.y - 10),
            width,
            5
        };
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
        
        healthBar.w = static_cast<int>((float)health / maxHealth * width);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);
    }
    
    void TakeDamage(int amount) {
        health -= amount;
        if (health <= 0) {
            health = 0;
            active = false;
        }
    }
    
    void ShootAt(Player* player, std::vector<std::shared_ptr<Projectile>>& projectiles) {
        if (player && player->active) {
            float dirX = (player->position.x > position.x) ? 1.0f : -1.0f;
            auto proj = std::make_shared<Projectile>(
                position.x + (dirX > 0 ? width : 0),
                position.y + height / 2,
                dirX,
                damage,
                false
            );
            projectiles.push_back(proj);
        }
    }
};

// SearchableItem class
class SearchableItem : public GameObject {
public:
    ItemType type;
    ArmorType armorType;
    int amount;
    bool searched;
    std::function<void(Player*, ItemType, ArmorType, int)> onSearchComplete;
    
    SearchableItem(float x, float y, ItemType itemType, int amt) : 
        GameObject(x, y, 24, 24),
        type(itemType),
        amount(amt),
        searched(false) {
        // For armor, randomly assign a piece type
        if (type == ItemType::ARMOR) {
            int randType = rand() % 5;
            switch(randType) {
                case 0: armorType = ArmorType::HEAD; break;
                case 1: armorType = ArmorType::CHEST; break;
                case 2: armorType = ArmorType::LEGS; break;
                case 3: armorType = ArmorType::ARMS; break;
                case 4: armorType = ArmorType::FEET; break;
            }
        }
    }
    
    void Render(SDL_Renderer* renderer) override {
        if (!active || searched) return;
        
        // Colors based on item type
        if (type == ItemType::AMMO) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
        } else if (type == ItemType::ARMOR) {
            SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255); // Silver
        } else if (type == ItemType::HEALTH) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        }
        
        SDL_Rect rect = GetRect();
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw a "?" to indicate searchable
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        int centerX = static_cast<int>(position.x + width / 2);
        int centerY = static_cast<int>(position.y + height / 2);
        
        // Draw "?" in a very simple way
        SDL_RenderDrawLine(renderer, centerX - 5, centerY - 8, centerX + 5, centerY - 8);
        SDL_RenderDrawLine(renderer, centerX + 5, centerY - 8, centerX + 5, centerY - 4);
        SDL_RenderDrawLine(renderer, centerX + 5, centerY - 4, centerX, centerY);
        SDL_RenderDrawLine(renderer, centerX, centerY, centerX, centerY + 4);
        SDL_RenderDrawPoint(renderer, centerX, centerY + 8);
    }
    
    void Complete(Player* player) {
        if (onSearchComplete) {
            onSearchComplete(player, type, armorType, amount);
        }
        searched = true;
        active = false;
    }
};

// Game class to manage everything
class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    GameState state;
    
    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Platform>> platforms;
    std::vector<std::shared_ptr<Ladder>> ladders;
    std::vector<std::shared_ptr<Enemy>> enemies;
    std::vector<std::shared_ptr<Projectile>> projectiles;
    std::vector<std::shared_ptr<SearchableItem>> searchItems;
    
    Uint32 lastFrameTime;
    float deltaTime;
    
    void HandleCollisions();
    void LoadLevel(int levelNum);
    
public:
    Game() : window(nullptr), renderer(nullptr), state(GameState::RUNNING), lastFrameTime(0), deltaTime(0) {}
    
    ~Game() {
        Cleanup();
    }
    
    bool Initialize() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create window
        window = SDL_CreateWindow("Platform Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                 SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_image
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return false;
        }
        
        // Initialize game
        lastFrameTime = SDL_GetTicks();
        LoadLevel(1);
        
        return true;
    }
    
    void HandleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                state = GameState::GAME_OVER;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        state = GameState::GAME_OVER;
                        break;
                    case SDLK_p:
                        if (state == GameState::RUNNING) {
                            state = GameState::PAUSED;
                        } else if (state == GameState::PAUSED) {
                            state = GameState::RUNNING;
                        }
                        break;
                }
            }
        }
    }
    
    void Update() {
        if (state != GameState::RUNNING) return;
        
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        deltaTime = (currentTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentTime;
        
        // Get keyboard state
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        
        // Handle player input
        if (player && player->active) {
            // Movement
            player->velocity.x = 0;
            if (keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A]) {
                player->velocity.x = -PLAYER_SPEED;
            }
            if (keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D]) {
                player->velocity.x = PLAYER_SPEED;
            }
            
            // Climbing ladders
            if (player->isOnLadder) {
                player->velocity.y = 0;
                if (keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W]) {
                    player->velocity.y = -PLAYER_SPEED;
                }
                if (keyState[SDL_SCANCODE_DOWN] || keyState[SDL_SCANCODE_S]) {
                    player->velocity.y = PLAYER_SPEED;
                }
            } else if (!player->isOnGround) {
                // Apply gravity if not on ladder and not on ground
                player->velocity.y += GRAVITY;
            }
            
            // Jump
            if ((keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W]) && player->isOnGround) {
                player->Jump();
            }
            
            // Shoot
            if (keyState[SDL_SCANCODE_LCTRL] || keyState[SDL_SCANCODE_RCTRL]) {
                // TODO: Implement player shooting
            }
            
            // Search items
            bool spacePressed = keyState[SDL_SCANCODE_SPACE] != 0;
            
            if (spacePressed && !player->isSearching) {
                // Check if player is near a searchable item
                for (auto& item : searchItems) {
                    if (item->active && !item->searched && player->CheckCollision(item.get())) {
                        player->StartSearching(item.get());
                        break;
                    }
                }
            } else if (!spacePressed && player->isSearching) {
                player->StopSearching();
            }
            
            if (player->isSearching) {
                if (player->UpdateSearching()) {
                    // Search completed
                    if (player->currentSearchItem) {
                        for (auto& item : searchItems) {
                            if (item.get() == player->currentSearchItem) {
                                item->Complete(player.get());
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        // Update all game objects
        if (player) player->Update(deltaTime);
        
        for (auto& enemy : enemies) {
            if (enemy->active) {
                enemy->Update(deltaTime, player.get(), projectiles);
            }
        }
        
        for (auto& proj : projectiles) {
            if (proj->active) {
                proj->Update(deltaTime);
            }
        }
        
        // Handle all collisions
        HandleCollisions();
        
        // Clean up inactive projectiles
        projectiles.erase(
            std::remove_if(
                projectiles.begin(),
                projectiles.end(),
                [](const std::shared_ptr<Projectile>& p) { return !p->active; }
            ),
            projectiles.end()
        );
        
        // Clean up inactive enemies
        enemies.erase(
            std::remove_if(
                enemies.begin(),
                enemies.end(),
                [](const std::shared_ptr<Enemy>& e) { return !e->active; }
            ),
            enemies.end()
        );
        
        // Clean up searched items
        searchItems.erase(
            std::remove_if(
                searchItems.begin(),
                searchItems.end(),
                [](const std::shared_ptr<SearchableItem>& s) { return !s->active; }
            ),
            searchItems.end()
        );
        
        // Check game over condition
        if (player && !player->active) {
            // TODO: Handle game over
        }
        
        // Check if level is cleared (all enemies defeated)
        if (enemies.empty()) {
            // TODO: Load next level
        }
    }
    
    void Render() {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Render all game objects
        for (auto& platform : platforms) {
            platform->Render(renderer);
        }
        
        for (auto& ladder : ladders) {
            ladder->Render(renderer);
        }
        
        for (auto& item : searchItems) {
            if (item->active) {
                item->Render(renderer);
            }
        }
        
        for (auto& proj : projectiles) {
            if (proj->active) {
                proj->Render(renderer);
            }
        }
        
        for (auto& enemy : enemies) {
            if (enemy->active) {
                enemy->Render(renderer);
            }
        }
        
        if (player && player->active) {
            player->Render(renderer);
        }
        
        // Render UI
        if (player) {
            // Render ammo
            std::string ammoText = "Ammo: " + std::to_string(player->ammo);
            // (In a real game, you'd use SDL_ttf to render text)
            
            // Render armor status
            std::string armorText = "Armor: " + std::to_string(player->armorLevel);
            // (In a real game, you'd use SDL_ttf to render text)
        }
        
        // Render pause screen
        if (state == GameState::PAUSED) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
            SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderFillRect(renderer, &overlay);
            // Render "PAUSED" text (in a real game, you'd use SDL_ttf)
        }
        
        // Update screen
        SDL_RenderPresent(renderer);
    }
    
    void Run() {
        while (state != GameState::GAME_OVER) {
            HandleEvents();
            Update();
            Render();
            
            // Cap frame rate
            SDL_Delay(16); // Approximately 60 FPS
        }
    }
    
    void Cleanup() {
        // Clear game objects
        platforms.clear();
        ladders.clear();
        enemies.clear();
        projectiles.clear();
        searchItems.clear();
        player.reset();
        
        // Destroy renderer and window
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        
        // Quit SDL systems
        IMG_Quit();
        SDL_Quit();
    }
};

// Implementation of Player's Update method
void Player::Update(float deltaTime) {
    Vector2 oldPosition = position;
    
    // Apply velocity
    position += velocity;
    
    // Reset ground state
    isOnGround = false;
    isOnLadder = false;
    
    // Player will implement its own gravity in the main game loop
}

bool Player::UpdateSearching() {
    if (!isSearching || !currentSearchItem) return false;
    
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = currentTime - searchStartTime;
    
    // Update progress (0.0 to 1.0)
    searchProgress = static_cast<float>(elapsedTime) / SEARCH_TIME;
    
    // Check if search is complete
    if (elapsedTime >= SEARCH_TIME) {
        isSearching = false;
        searchProgress = 1.0f;
        return true;
    }
    
    return false;
}

// Implementation of Player's Shoot method
void Player::Shoot() {
    if (ammo <= 0) return;
    
    // Determine direction based on player orientation (simplified)
    float direction = 1.0f; // Right by default, in practice would depend on player facing direction
    
    // Create new projectile
    // In actual implementation, this would add to Game's projectiles vector
    
    // Decrease ammo
    ammo--;
}

// Implementation of Enemy's Update method
void Enemy::Update(float deltaTime, Player* player, std::vector<std::shared_ptr<Projectile>>& projectiles) {
    // Update shot cooldown
    timeSinceLastShot += deltaTime;
    
    // Check if player is in range
    bool playerInRange = false;
    if (player && player->active) {
        float dx = player->position.x - position.x;
        float dy = player->position.y - position.y;
        float distSq = dx * dx + dy * dy;
        
        playerInRange = distSq < (playerDetectionRange * playerDetectionRange);
        
        // If player is in range, shoot if cooldown is ready
        if (playerInRange && timeSinceLastShot >= shootCooldown) {
            ShootAt(player, projectiles);
            timeSinceLastShot = 0;
        }
    }
    
    // Simple movement AI
    moveTimer += deltaTime;
    if (moveTimer > 2.0f) {
        moveDirection *= -1;
        moveTimer = 0;
    }
    
    // Move left/right
    velocity.x = moveDirection * 2.0f;
    
    // Apply velocity
    position += velocity;
}

// Implementation of Game's HandleCollisions method
void Game::HandleCollisions() {
    if (!player || !player->active) return;
    
    // Player-Platform collisions
    player->isOnGround = false;
    for (auto& platform : platforms) {
        if (player->CheckCollision(platform.get())) {
            // Check if landing on top of platform
            if (player->velocity.y > 0 && 
                player->position.y + player->height - player->velocity.y <= platform->position.y) {
                player->position.y = platform->position.y - player->height;
                player->velocity.y = 0;
                player->isOnGround = true;
            }
            // Check for horizontal collision
            else if (player->velocity.x != 0) {
                player->position.x -= player->velocity.x;
            }
        }
    }
    
    // Player-Ladder collisions
    for (auto& ladder : ladders) {
        if (player->CheckCollision(ladder.get())) {
            player->isOnLadder = true;
            break;
        }
    }
    
    // Enemy-Platform collisions
    for (auto& enemy : enemies) {
        if (!enemy->active) continue;
        
        bool enemyOnGround = false;
        for (auto& platform : platforms) {
            if (enemy->CheckCollision(platform.get())) {  // Missing closing parenthesis and .get() call
                // Check if landing on top of platform
                if (enemy->velocity.y > 0 && 
                    enemy->position.y + enemy->height - enemy->velocity.y <= platform->position.y) {
                    enemy->position.y = platform->position.y - enemy->height;
                    enemy->velocity.y = 0;
                    enemyOnGround = true;
                }
                // Check for horizontal collision
                else if (enemy->velocity.x != 0) {
                    enemy->position.x -= enemy->velocity.x;
                    enemy->moveDirection *= -1; // Reverse direction
                }
            }
        }
        
        // Apply gravity if not on ground
        if (!enemyOnGround) {
            enemy->velocity.y += GRAVITY;
        } else {
            enemy->velocity.y = 0;
        }
    }
    
    // Projectile collisions
    for (auto& proj : projectiles) {
        if (!proj->active) continue;
        
        // Check projectile-platform collisions
        for (auto& platform : platforms) {
            if (proj->CheckCollision(platform.get())) {
                proj->active = false;
                break;
            }
        }
        
        if (!proj->active) continue;
        
        // Check projectile-player collisions (enemy projectiles)
        if (!proj->fromPlayer && player->active && proj->CheckCollision(player.get())) {
            player->TakeDamage(proj->damage);
            proj->active = false;
            continue;
        }
        
        // Check projectile-enemy collisions (player projectiles)
        if (proj->fromPlayer) {
            for (auto& enemy : enemies) {
                if (enemy->active && proj->CheckCollision(enemy.get())) {
                    enemy->TakeDamage(proj->damage);
                    proj->active = false;
                    break;
                }
            }
        }
    }
}
// Implementation of Game's LoadLevel method
void Game::LoadLevel(int levelNum) {
    // Clear previous level
    platforms.clear();
    ladders.clear();
    enemies.clear();
    projectiles.clear();
    searchItems.clear();
    
    // Create player
    player = std::make_shared<Player>(100, 400);
    
    // Create platforms
    platforms.push_back(std::make_shared<Platform>(0, 500, 800, 20));  // Ground
    platforms.push_back(std::make_shared<Platform>(100, 400, 200, 20)); // Platform 1
    platforms.push_back(std::make_shared<Platform>(400, 350, 200, 20)); // Platform 2
    platforms.push_back(std::make_shared<Platform>(200, 250, 200, 20)); // Platform 3
    
    // Create ladders
    ladders.push_back(std::make_shared<Ladder>(150, 400, 30, 100)); // Ground to Platform 1
    ladders.push_back(std::make_shared<Ladder>(450, 350, 30, 150)); // Platform 2 to Ground
    ladders.push_back(std::make_shared<Ladder>(250, 250, 30, 150)); // Platform 3 to Platform 1
    
    // Create enemies
    enemies.push_back(std::make_shared<Enemy>(300, 368, 50)); // On Platform 1
    enemies.push_back(std::make_shared<Enemy>(500, 318, 75)); // On Platform 2
    enemies.push_back(std::make_shared<Enemy>(300, 218, 100)); // On Platform 3
    
    // Create searchable items
    // Ammo items
    searchItems.push_back(std::make_shared<SearchableItem>(150, 376, ItemType::AMMO, 10));
    searchItems.push_back(std::make_shared<SearchableItem>(550, 326, ItemType::AMMO, 15));
    
    // Health items
    searchItems.push_back(std::make_shared<SearchableItem>(200, 376, ItemType::HEALTH, 20));
    searchItems.push_back(std::make_shared<SearchableItem>(350, 226, ItemType::HEALTH, 30));
    
    // Armor items
    searchItems.push_back(std::make_shared<SearchableItem>(250, 376, ItemType::ARMOR, 0));
    searchItems.push_back(std::make_shared<SearchableItem>(450, 326, ItemType::ARMOR, 0));
    searchItems.push_back(std::make_shared<SearchableItem>(250, 226, ItemType::ARMOR, 0));
    searchItems.push_back(std::make_shared<SearchableItem>(350, 476, ItemType::ARMOR, 0));
    searchItems.push_back(std::make_shared<SearchableItem>(550, 476, ItemType::ARMOR, 0));
    
    // Set up callback for search items
    for (auto& item : searchItems) {
        item->onSearchComplete = [](Player* player, ItemType type, ArmorType armorType, int amount) {
            if (type == ItemType::AMMO) {
                player->ammo += amount;
            } else if (type == ItemType::HEALTH) {
                player->health += amount;
                if (player->health > player->maxHealth) {
                    player->health = player->maxHealth;
                }
            } else if (type == ItemType::ARMOR) {
                player->AddArmorPiece(armorType);
            }
        };
    }
}

// Main function
int main(int argc, char* argv[]) {
    Game game;
    
    if (!game.Initialize()) {
        std::cout << "Failed to initialize game!" << std::endl;
        return -1;
    }
    
    game.Run();
    
    return 0;
}
