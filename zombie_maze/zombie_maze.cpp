#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <random>
#include <ctime>
#include <memory>
#include <algorithm>
#include <string>
#include <iostream>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 32;
const int PLAYER_SPEED = 4;
const int ZOMBIE_SPEED = 2;
const int BULLET_SPEED = 10;
const int DETECTION_RADIUS = 150;
const int ZOMBIE_RESPAWN_TIME = 15 * 60; // 15 seconds * 60 frames per second

// Game states
enum class GameState {
    PLAYING,
    NEXT_LEVEL,
    GAME_OVER
};

// Entity types
enum class EntityType {
    PLAYER,
    ZOMBIE,
    KEY,
    WALL,
    BULLET
};

struct Position {
    float x, y;
    
    Position(float x = 0, float y = 0) : x(x), y(y) {}
    
    float distance(const Position& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

struct Velocity {
    float x, y;
    
    Velocity(float x = 0, float y = 0) : x(x), y(y) {}
};

// Base entity class
class Entity {
public:
    Entity(EntityType type, Position pos, int width, int height)
        : type(type), position(pos), width(width), height(height), active(true) {}
        
    virtual ~Entity() {}
    
    virtual void update() {}
    virtual void render(SDL_Renderer* renderer, SDL_Texture* texture) = 0;
    
    SDL_Rect getRect() const {
        return {static_cast<int>(position.x), static_cast<int>(position.y), width, height};
    }
    
    bool collidesWith(const Entity& other) const {
        SDL_Rect rect1 = getRect();
        SDL_Rect rect2 = other.getRect();
        
        return SDL_HasIntersection(&rect1, &rect2);
    }
    
    EntityType getType() const { return type; }
    Position getPosition() const { return position; }
    bool isActive() const { return active; }
    void setActive(bool value) { active = value; }
    
protected:
    EntityType type;
    Position position;
    int width, height;
    bool active;
};

// Player entity
class Player : public Entity {
public:
    Player(Position pos)
        : Entity(EntityType::PLAYER, pos, TILE_SIZE, TILE_SIZE),
          velocity({0, 0}), keys(0) {}
    
    void update() override {
        position.x += velocity.x;
        position.y += velocity.y;
    }
    
    void render(SDL_Renderer* renderer, SDL_Texture* texture) override {
        SDL_Rect srcRect = {0, 0, TILE_SIZE, TILE_SIZE};
        SDL_Rect destRect = getRect();
        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
    
    void setVelocity(const Velocity& vel) { velocity = vel; }
    int getKeys() const { return keys; }
    void collectKey() { keys++; }
    void resetKeys() { keys = 0; }
    
private:
    Velocity velocity;
    int keys;
};

// Zombie entity
class Zombie : public Entity {
public:
    Zombie(Position pos, int level)
        : Entity(EntityType::ZOMBIE, pos, TILE_SIZE, TILE_SIZE),
          velocity({0, 0}), state(ZombieState::PATROL),
          patrolTimer(0), respawnTimer(0), respawning(false),
          health(level * 2), maxHealth(level * 2),
          spawnPosition(pos) {}
    
    enum class ZombieState {
        PATROL,
        CHASE,
        IDLE,
        RESPAWNING
    };
    
    void update() override {
        if (respawning) {
            respawnTimer++;
            if (respawnTimer >= ZOMBIE_RESPAWN_TIME) {
                respawnTimer = 0;
                respawning = false;
                active = true;
                position = spawnPosition;
                health = maxHealth;
                state = ZombieState::PATROL;
            }
            return;
        }
        
        position.x += velocity.x;
        position.y += velocity.y;
        
        // Update patrol behavior
        if (state == ZombieState::PATROL) {
            patrolTimer++;
            if (patrolTimer > 120) { // Change direction every 2 seconds
                patrolTimer = 0;
                // Random direction change
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dirDist(0, 3);
                int direction = dirDist(gen);
                
                velocity = {0, 0};
                switch (direction) {
                    case 0: velocity.x = ZOMBIE_SPEED; break;
                    case 1: velocity.x = -ZOMBIE_SPEED; break;
                    case 2: velocity.y = ZOMBIE_SPEED; break;
                    case 3: velocity.y = -ZOMBIE_SPEED; break;
                }
            }
        }
    }
    
    void render(SDL_Renderer* renderer, SDL_Texture* texture) override {
        if (!active) return;
        
        SDL_Rect srcRect = {TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
        SDL_Rect destRect = getRect();
        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
        
        // Render health bar
        int healthBarWidth = width;
        int healthBarHeight = 5;
        float healthPercentage = static_cast<float>(health) / maxHealth;
        
        SDL_Rect healthBgRect = {
            static_cast<int>(position.x),
            static_cast<int>(position.y - 10),
            healthBarWidth,
            healthBarHeight
        };
        
        SDL_Rect healthRect = {
            static_cast<int>(position.x),
            static_cast<int>(position.y - 10),
            static_cast<int>(healthBarWidth * healthPercentage),
            healthBarHeight
        };
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBgRect);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &healthRect);
    }
    
    void chase(const Position& targetPos) {
        if (!active || respawning) return;
        
        state = ZombieState::CHASE;
        
        float dx = targetPos.x - position.x;
        float dy = targetPos.y - position.y;
        float length = std::sqrt(dx * dx + dy * dy);
        
        if (length > 0) {
            dx /= length;
            dy /= length;
            
            velocity.x = dx * ZOMBIE_SPEED;
            velocity.y = dy * ZOMBIE_SPEED;
        }
    }
    
    void patrol() {
        if (!active || respawning) return;
        
        state = ZombieState::PATROL;
    }
    
    void takeDamage(int amount) {
        health -= amount;
        if (health <= 0) {
            die();
        }
    }
    
    void die() {
        active = false;
        respawning = true;
        respawnTimer = 0;
    }
    
    bool isRespawning() const { return respawning; }
    ZombieState getState() const { return state; }
    
private:
    Velocity velocity;
    ZombieState state;
    int patrolTimer;
    int respawnTimer;
    bool respawning;
    int health;
    int maxHealth;
    Position spawnPosition;
};

// Key entity
class Key : public Entity {
public:
    Key(Position pos)
        : Entity(EntityType::KEY, pos, TILE_SIZE, TILE_SIZE) {}
    
    void render(SDL_Renderer* renderer, SDL_Texture* texture) override {
        if (!active) return;
        
        SDL_Rect srcRect = {TILE_SIZE * 2, 0, TILE_SIZE, TILE_SIZE};
        SDL_Rect destRect = getRect();
        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
};

// Wall entity
class Wall : public Entity {
public:
    Wall(Position pos)
        : Entity(EntityType::WALL, pos, TILE_SIZE, TILE_SIZE) {}
    
    void render(SDL_Renderer* renderer, SDL_Texture* texture) override {
        SDL_Rect srcRect = {TILE_SIZE * 3, 0, TILE_SIZE, TILE_SIZE};
        SDL_Rect destRect = getRect();
        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
};

// Bullet entity
class Bullet : public Entity {
public:
    Bullet(Position pos, float dirX, float dirY)
        : Entity(EntityType::BULLET, pos, 8, 8),
          velocity({dirX * BULLET_SPEED, dirY * BULLET_SPEED}),
          lifetime(120) {} // Bullet lifetime: 2 seconds
    
    void update() override {
        position.x += velocity.x;
        position.y += velocity.y;
        
        lifetime--;
        if (lifetime <= 0) {
            active = false;
        }
    }
    
    void render(SDL_Renderer* renderer, SDL_Texture* texture) override {
        SDL_Rect srcRect = {TILE_SIZE * 4, 0, 8, 8};
        SDL_Rect destRect = getRect();
        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
    
private:
    Velocity velocity;
    int lifetime;
};

// Level generator
class Level {
public:
    Level(int levelNumber)
        : levelNumber(levelNumber),
          width(20 + levelNumber),
          height(15 + levelNumber),
          totalKeys(3 + levelNumber / 2),
          totalZombies(2 + levelNumber) {
        
        // Initialize grid with empty spaces
        grid.resize(height, std::vector<int>(width, 0));
        generateMaze();
    }
    
    void generateMaze() {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Fill with walls
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x] = 1; // Wall
            }
        }
        
        // Start carving from the center
        int startX = width / 2;
        int startY = height / 2;
        grid[startY][startX] = 0; // Empty space
        
        // Recursive backtracking maze generation
        std::vector<std::pair<int, int>> stack;
        stack.push_back({startX, startY});
        
        while (!stack.empty()) {
            int x = stack.back().first;
            int y = stack.back().second;
            
            // Define directions: right, left, down, up
            int dx[4] = {2, -2, 0, 0};
            int dy[4] = {0, 0, 2, -2};
            
            // Shuffle directions
            std::vector<int> directions = {0, 1, 2, 3};
            std::shuffle(directions.begin(), directions.end(), gen);
            
            bool moved = false;
            
            for (int dir : directions) {
                int nx = x + dx[dir];
                int ny = y + dy[dir];
                
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && grid[ny][nx] == 1) {
                    // Carve path
                    grid[y + dy[dir]/2][x + dx[dir]/2] = 0;
                    grid[ny][nx] = 0;
                    
                    stack.push_back({nx, ny});
                    moved = true;
                    break;
                }
            }
            
            if (!moved) {
                stack.pop_back();
            }
        }
        
        // Create rooms by randomly removing walls
        std::uniform_int_distribution<> roomDist(0, 100);
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                if (grid[y][x] == 1 && roomDist(gen) < 20) {
                    grid[y][x] = 0;
                }
            }
        }
        
        // Ensure the maze is connected
        ensureConnectivity();
        
        // Place player at a random empty space near the edge
        std::uniform_int_distribution<> edgeDist(1, 3);
        int playerX, playerY;
        do {
            playerX = edgeDist(gen);
            playerY = edgeDist(gen);
        } while (grid[playerY][playerX] != 0);
        
        playerPosition = {playerX * TILE_SIZE, playerY * TILE_SIZE};
        
        // Place keys in random locations
        placeEntities(2, totalKeys, keyPositions); // type 2 = key
        
        // Place zombies in random locations
        placeEntities(3, totalZombies, zombiePositions); // type 3 = zombie
    }
    
    void ensureConnectivity() {
        // Find all connected components and connect them
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        std::vector<std::vector<std::pair<int, int>>> components;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (grid[y][x] == 0 && !visited[y][x]) {
                    std::vector<std::pair<int, int>> component;
                    std::vector<std::pair<int, int>> stack = {{x, y}};
                    visited[y][x] = true;
                    
                    while (!stack.empty()) {
                        auto [cx, cy] = stack.back();
                        stack.pop_back();
                        component.push_back({cx, cy});
                        
                        int dx[4] = {1, -1, 0, 0};
                        int dy[4] = {0, 0, 1, -1};
                        
                        for (int dir = 0; dir < 4; dir++) {
                            int nx = cx + dx[dir];
                            int ny = cy + dy[dir];
                            
                            if (nx >= 0 && nx < width && ny >= 0 && ny < height && 
                                grid[ny][nx] == 0 && !visited[ny][nx]) {
                                stack.push_back({nx, ny});
                                visited[ny][nx] = true;
                            }
                        }
                    }
                    
                    components.push_back(component);
                }
            }
        }
        
        // If there's more than one component, connect them
        if (components.size() > 1) {
            std::random_device rd;
            std::mt19937 gen(rd());
            
            for (size_t i = 1; i < components.size(); i++) {
                std::uniform_int_distribution<> dist1(0, components[0].size() - 1);
                std::uniform_int_distribution<> dist2(0, components[i].size() - 1);
                
                auto [x1, y1] = components[0][dist1(gen)];
                auto [x2, y2] = components[i][dist2(gen)];
                
                // Connect these two points
                int sx = x1 < x2 ? x1 : x2;
                int ex = x1 < x2 ? x2 : x1;
                int sy = y1 < y2 ? y1 : y2;
                int ey = y1 < y2 ? y2 : y1;
                
                for (int x = sx; x <= ex; x++) {
                    grid[y1][x] = 0;
                }
                
                for (int y = sy; y <= ey; y++) {
                    grid[y][x2] = 0;
                }
            }
        }
    }
    
    void placeEntities(int entityType, int count, std::vector<Position>& positions) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> xDist(1, width - 2);
        std::uniform_int_distribution<> yDist(1, height - 2);
        
        for (int i = 0; i < count; i++) {
            int x, y;
            do {
                x = xDist(gen);
                y = yDist(gen);
                
                // Check if it's far enough from player start
                float dist = std::sqrt(std::pow(x * TILE_SIZE - playerPosition.x, 2) + 
                                      std::pow(y * TILE_SIZE - playerPosition.y, 2));
                
            } while (grid[y][x] != 0 || dist < 5 * TILE_SIZE);
            
            positions.push_back({x * TILE_SIZE, y * TILE_SIZE});
        }
    }
    
    std::vector<std::shared_ptr<Entity>> createEntities() const {
        std::vector<std::shared_ptr<Entity>> entities;
        
        // Add walls
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (grid[y][x] == 1) {
                    entities.push_back(std::make_shared<Wall>(Position(x * TILE_SIZE, y * TILE_SIZE)));
                }
            }
        }
        
        // Add keys
        for (const auto& pos : keyPositions) {
            entities.push_back(std::make_shared<Key>(pos));
        }
        
        // Add zombies
        for (const auto& pos : zombiePositions) {
            entities.push_back(std::make_shared<Zombie>(pos, levelNumber));
        }
        
        return entities;
    }
    
    Position getPlayerStartPosition() const {
        return playerPosition;
    }
    
    int getKeyCount() const {
        return totalKeys;
    }
    
private:
    int levelNumber;
    int width;
    int height;
    int totalKeys;
    int totalZombies;
    std::vector<std::vector<int>> grid; // 0 = empty, 1 = wall
    Position playerPosition;
    std::vector<Position> keyPositions;
    std::vector<Position> zombiePositions;
};

// Game class
class Game {
public:
    Game()
        : window(nullptr), renderer(nullptr), running(false),
          state(GameState::PLAYING), currentLevel(1) {}
    
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow("Zombie Maze", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_image
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return false;
        }
        
        // Load textures
        texture = loadTexture("sprites.png");
        if (!texture) {
            std::cerr << "Failed to load texture!" << std::endl;
            return false;
        }
        
        loadLevel(currentLevel);
        running = true;
        
        return true;
    }
    
    void loadLevel(int level) {
        Level levelGenerator(level);
        entities = levelGenerator.createEntities();
        
        // Create player
        player = std::make_shared<Player>(levelGenerator.getPlayerStartPosition());
        entities.push_back(player);
        
        keysRequired = levelGenerator.getKeyCount();
    }
    
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_SPACE:
                        if (state == GameState::NEXT_LEVEL) {
                            currentLevel++;
                            loadLevel(currentLevel);
                            state = GameState::PLAYING;
                        }
                        break;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                // Shoot bullet
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                
                Position playerPos = player->getPosition();
                playerPos.x += TILE_SIZE / 2;
                playerPos.y += TILE_SIZE / 2;
                
                float dx = mouseX - playerPos.x;
                float dy = mouseY - playerPos.y;
                float length = std::sqrt(dx * dx + dy * dy);
                
                if (length > 0) {
                    dx /= length;
                    dy /= length;
                    
                    Position bulletPos = {playerPos.x, playerPos.y};
                    entities.push_back(std::make_shared<Bullet>(bulletPos, dx, dy));
                }
            }
        }
        
        // Handle keyboard input for player movement
        const Uint8* keyState = SDL_GetKeyboardState(NULL);
        float velocityX = 0, velocityY = 0;
        
        if (keyState[SDL_SCANCODE_W]) velocityY = -PLAYER_SPEED;
        if (keyState[SDL_SCANCODE_S]) velocityY = PLAYER_SPEED;
        if (keyState[SDL_SCANCODE_A]) velocityX = -PLAYER_SPEED;
        if (keyState[SDL_SCANCODE_D]) velocityX = PLAYER_SPEED;
        
        // Normalize diagonal movement
        if (velocityX != 0 && velocityY != 0) {
            float length = std::sqrt(velocityX * velocityX + velocityY * velocityY);
            velocityX = velocityX / length * PLAYER_SPEED;
            velocityY = velocityY / length * PLAYER_SPEED;
        }
        
        player->setVelocity({velocityX, velocityY});
    }
    
    void update() {
        if (state != GameState::PLAYING) return;
        
        // Update entities
        for (auto& entity : entities) {
            entity->update();
        }
        
        // Handle collisions
        handleCollisions();
        
        // Update zombie AI
        Position playerPos = player->getPosition();
        for (auto& entity : entities) {
            if (entity->getType() == EntityType::ZOMBIE) {
                auto zombie = std::static_pointer_cast<Zombie>(entity);
                if (!zombie->isActive() || zombie->isRespawning()) continue;
                
                float distance = playerPos.distance(zombie->getPosition());
                if (distance <= DETECTION_RADIUS) {
                    zombie->chase(playerPos);
                } else if (zombie->getState() == Zombie::ZombieState::CHASE) {
                    zombie->patrol();
                }
            }
        }
        
        // Check if all keys are collected
        if (player->getKeys() >= keysRequired) {
            state = GameState::NEXT_LEVEL;
        }
        
        // Remove inactive entities
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [](const std::shared_ptr<Entity>& e) {
                    return e->getType() == EntityType::BULLET && !e->isActive();
                }),
            entities.end()
        );
    }
    
    void handleCollisions() {
        // Player collision with walls
        Position originalPos = player->getPosition();
        player->update();
        
        for (auto& entity : entities) {
            if (entity->getType() == EntityType::WALL && player->collidesWith(*entity)) {
                // Restore original position if collision with wall
                player->setVelocity({0, 0});
                player->getPosition() = originalPos;
                break;
            }
        }
        
        // Player collision with keys
        for (auto& entity : entities) {
            if (entity->getType() == EntityType::KEY && entity->isActive() && player->collidesWith(*entity)) {
                entity->setActive(false);
                player->collectKey();
            }
        }
        
        // Bullet collisions
        for (auto& bullet : entities) {
            if (bullet->getType() != EntityType::BULLET || !bullet->isActive()) continue;
            
            // Check collision with walls
            for (auto& wall : entities) {
                if (wall->getType() == EntityType::WALL && bullet->collidesWith(*wall)) {
                    bullet->setActive(false);
                    break;
                }
            }
            
            // Check collision with zombies
            for (auto& zombie : entities) {
                if (zombie->getType() == EntityType::ZOMBIE && zombie->isActive() && bullet->collidesWith(*zombie)) {
                    auto zombiePtr = std::static_pointer_cast<Zombie>(zombie);
                    zombiePtr->takeDamage(1);
                    bullet->setActive(false);
                    break;
                }
            }
        }
        
        // Player collision with zombies
        for (auto& entity : entities) {
            if (entity->getType() == EntityType::ZOMBIE && 
                entity->isActive() && player->collidesWith(*entity)) {
                // Game over logic could go here
                // For now, just push the player away
                Position zombiePos = entity->getPosition();
                Position playerPos = player->getPosition();
                
                float dx = playerPos.x - zombiePos.x;
                float dy = playerPos.y - zombiePos.y;
                float length = std::sqrt(dx * dx + dy * dy);
                
                if (length > 0) {
                    dx /= length;
                    dy /= length;
                    player->setVelocity({dx * PLAYER_SPEED * 2, dy * PLAYER_SPEED * 2});
                }
            }
        }
        
        // Zombie collision with walls
        for (auto& entity : entities) {
            if (entity->getType() != EntityType::ZOMBIE || !entity->isActive()) continue;
            
            auto zombie = std::static_pointer_cast<Zombie>(entity);
            Position originalZombiePos = zombie->getPosition();
            zombie->update();
            
            bool collision = false;
            for (auto& wall : entities) {
                if (wall->getType() == EntityType::WALL && zombie->collidesWith(*wall)) {
                    collision = true;
                    break;
                }
            }
            
            if (collision) {
                zombie->getPosition() = originalZombiePos;
                // Change direction for patrol
                if (zombie->getState() == Zombie::ZombieState::PATROL) {
                    zombie->patrol();
                }
            }
        }
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Render all entities
        for (auto& entity : entities) {
            if (entity->getType() != EntityType::PLAYER) { // Render player last
                entity->render(renderer, texture);
            }
        }
        
        // Render player on top
        player->render(renderer, texture);
        
        // Render UI
        renderUI();
        
        SDL_RenderPresent(renderer);
    }
    
    void renderUI() {
        // Render key count
        std::string keyText = "Keys: " + std::to_string(player->getKeys()) + "/" + std::to_string(keysRequired);
        renderText(keyText, 10, 10);
        
        // Render level number
        std::string levelText = "Level: " + std::to_string(currentLevel);
        renderText(levelText, SCREEN_WIDTH - 100, 10);
        
        // Render game state messages
        if (state == GameState::NEXT_LEVEL) {
            std::string message = "Level Complete! Press SPACE to continue to level " + std::to_string(currentLevel + 1);
            renderText(message, SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2);
        } else if (state == GameState::GAME_OVER) {
            renderText("Game Over! Press SPACE to restart", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2);
        }
    }
    
    void renderText(const std::string& text, int x, int y) {
        // In a real implementation, you would use SDL_ttf to render text
        // For simplicity, we'll just draw a rectangle as a placeholder
        SDL_Rect textRect = {x, y, static_cast<int>(text.length() * 8), 20};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &textRect);
    }
    
    void close() {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        
        IMG_Quit();
        SDL_Quit();
    }
    
    bool isRunning() const {
        return running;
    }
    
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    bool running;
    GameState state;
    int currentLevel;
    int keysRequired;
    
    std::vector<std::shared_ptr<Entity>> entities;
    std::shared_ptr<Player> player;
SDL_Texture* loadTexture(const std::string& path) {
        // In a real implementation, you would load the texture from file
        // For this example, we'll create a simple texture programmatically
        SDL_Surface* surface = SDL_CreateRGBSurface(0, TILE_SIZE * 5, TILE_SIZE, 32,
                                                    0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        if (!surface) {
            std::cerr << "Failed to create surface! SDL Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        // Draw player sprite (blue square)
        SDL_Rect playerRect = {0, 0, TILE_SIZE, TILE_SIZE};
        SDL_FillRect(surface, &playerRect, SDL_MapRGB(surface->format, 0, 0, 255));
        
        // Draw zombie sprite (red square)
        SDL_Rect zombieRect = {TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
        SDL_FillRect(surface, &zombieRect, SDL_MapRGB(surface->format, 255, 0, 0));
        
        // Draw key sprite (yellow square)
        SDL_Rect keyRect = {TILE_SIZE * 2, 0, TILE_SIZE, TILE_SIZE};
        SDL_FillRect(surface, &keyRect, SDL_MapRGB(surface->format, 255, 255, 0));
        
        // Draw wall sprite (gray square)
        SDL_Rect wallRect = {TILE_SIZE * 3, 0, TILE_SIZE, TILE_SIZE};
        SDL_FillRect(surface, &wallRect, SDL_MapRGB(surface->format, 128, 128, 128));
        
        // Draw bullet sprite (white square)
        SDL_Rect bulletRect = {TILE_SIZE * 4, 0, 8, 8};
        SDL_FillRect(surface, &bulletRect, SDL_MapRGB(surface->format, 255, 255, 255));
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        return texture;
    }
};

// Main function
int main(int argc, char* argv[]) {
    Game game;
    
    if (!game.init()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return -1;
    }
    
    // Main game loop
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    
    Uint32 frameStart;
    int frameTime;
    
    while (game.isRunning()) {
        frameStart = SDL_GetTicks();
        
        game.handleEvents();
        game.update();
        game.render();
        
        frameTime = SDL_GetTicks() - frameStart;
        
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
    
    game.close();
    
    return 0;
}
