// Top-Down GTA Clone using SDL2
// This is a foundation for a GTA-style game with:
// - Top-down view
// - Player character that can walk and drive vehicles
// - Basic NPC movement
// - Simple collision detection
// - Tile-based map system

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <memory>
#include <algorithm>
#include <random>
#include <ctime>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 64;
const float PLAYER_SPEED = 3.0f;
const float CAR_MAX_SPEED = 6.0f;
const float CAR_ACCELERATION = 0.1f;
const float CAR_DECELERATION = 0.05f;
const float CAR_TURN_SPEED = 3.0f;
const int MAP_WIDTH = 50;
const int MAP_HEIGHT = 50;

// Game states
enum GameState {
    STATE_WALKING,
    STATE_DRIVING
};

// Tile types
enum TileType {
    TILE_ROAD,
    TILE_GRASS,
    TILE_BUILDING,
    TILE_WATER
};

// Entity types
enum EntityType {
    ENTITY_PLAYER,
    ENTITY_NPC,
    ENTITY_CAR,
    ENTITY_POLICE_CAR
};

// Utility functions
float distance(float x1, float y1, float x2, float y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

// Vector2 struct for positions and velocities
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
    
    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }
    
    float magnitude() const {
        return sqrt(x * x + y * y);
    }
    
    Vector2 normalized() const {
        float mag = magnitude();
        if (mag > 0) {
            return Vector2(x / mag, y / mag);
        }
        return Vector2(0, 0);
    }
};

// Forward declarations
class Entity;
class Vehicle;
class Game;

// Camera class to handle viewport
class Camera {
public:
    Vector2 position;
    
    Camera() : position(0, 0) {}
    
    SDL_Rect getViewport(const Vector2& entityPos) {
        position.x = entityPos.x - SCREEN_WIDTH / 2;
        position.y = entityPos.y - SCREEN_HEIGHT / 2;
        
        // Clamp camera to map boundaries
        position.x = std::max(0.0f, std::min(position.x, (MAP_WIDTH * TILE_SIZE) - SCREEN_WIDTH + 0.0f));
        position.y = std::max(0.0f, std::min(position.y, (MAP_HEIGHT * TILE_SIZE) - SCREEN_HEIGHT + 0.0f));
        
        return {(int)position.x, (int)position.y, SCREEN_WIDTH, SCREEN_HEIGHT};
    }
};

// Base entity class
class Entity {
protected:
    SDL_Texture* texture;
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float speed;
    int width, height;
    bool collidable;
    EntityType type;
    
public:
    Entity(SDL_Texture* tex, float x, float y, int w, int h, EntityType t, bool collide = true) :
        texture(tex), position(x, y), velocity(0, 0), rotation(0), speed(0),
        width(w), height(h), collidable(collide), type(t) {}
    
    virtual ~Entity() {}
    
    virtual void update(float deltaTime, const std::vector<std::shared_ptr<Entity>>& entities, const std::vector<std::vector<TileType>>& map) {
        // Basic movement
        position = position + velocity * deltaTime;
    }
    
    virtual void render(SDL_Renderer* renderer, const SDL_Rect& camera) {
        SDL_Rect destRect = {
            (int)(position.x - camera.x - width / 2),
            (int)(position.y - camera.y - height / 2),
            width,
            height
        };
        
        // Only render if on screen
        if (destRect.x + destRect.w >= 0 && destRect.x <= SCREEN_WIDTH &&
            destRect.y + destRect.h >= 0 && destRect.y <= SCREEN_HEIGHT) {
            SDL_RenderCopyEx(renderer, texture, NULL, &destRect, rotation, NULL, SDL_FLIP_NONE);
        }
    }
    
    bool checkCollision(const Entity& other) const {
        if (!collidable || !other.collidable) return false;
        
        return distance(position.x, position.y, other.position.x, other.position.y) < 
               (width + other.width) / 2.5f;
    }
    
    bool checkTileCollision(const std::vector<std::vector<TileType>>& map) const {
        if (!collidable) return false;
        
        // Get surrounding tiles
        int tileX = (int)(position.x / TILE_SIZE);
        int tileY = (int)(position.y / TILE_SIZE);
        
        for (int y = tileY - 1; y <= tileY + 1; y++) {
            for (int x = tileX - 1; x <= tileX + 1; x++) {
                if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                    if (map[y][x] == TILE_BUILDING || map[y][x] == TILE_WATER) {
                        float tileLeft = x * TILE_SIZE;
                        float tileTop = y * TILE_SIZE;
                        
                        // Simple AABB collision
                        if (position.x + width / 2 > tileLeft && 
                            position.x - width / 2 < tileLeft + TILE_SIZE &&
                            position.y + height / 2 > tileTop && 
                            position.y - height / 2 < tileTop + TILE_SIZE) {
                            return true;
                        }
                    }
                }
            }
        }
        
        return false;
    }
    
    // Getters
    Vector2 getPosition() const { return position; }
    Vector2 getVelocity() const { return velocity; }
    float getRotation() const { return rotation; }
    EntityType getType() const { return type; }
    
    // For vehicle interactions
    virtual void setPosition(const Vector2& pos) { position = pos; }
    virtual void setRotation(float rot) { rotation = rot; }
};

// Vehicle class
class Vehicle : public Entity {
private:
    float currentSpeed;
    float maxSpeed;
    float acceleration;
    float deceleration;
    float turnSpeed;
    bool occupied;
    
public:
    Vehicle(SDL_Texture* tex, float x, float y, int w, int h, EntityType t) :
        Entity(tex, x, y, w, h, t), currentSpeed(0), maxSpeed(CAR_MAX_SPEED),
        acceleration(CAR_ACCELERATION), deceleration(CAR_DECELERATION),
        turnSpeed(CAR_TURN_SPEED), occupied(false) {}
    
    void update(float deltaTime, const std::vector<std::shared_ptr<Entity>>& entities, const std::vector<std::vector<TileType>>& map) override {
        // Save old position for collision resolution
        Vector2 oldPosition = position;
        
        if (!occupied) {
            // AI car behavior when not occupied
            // Simple car AI: follow roads and make random turns
            currentSpeed = maxSpeed * 0.5f;
            
            // Random direction changes occasionally
            static std::mt19937 rng(std::time(nullptr));
            std::uniform_int_distribution<int> turnDist(0, 100);
            if (turnDist(rng) < 1) {
                std::uniform_int_distribution<int> angleDist(0, 3);
                rotation = angleDist(rng) * 90.0f;
            }
        }
        
        // Apply physics
        float radians = rotation * M_PI / 180.0f;
        velocity.x = cos(radians) * currentSpeed;
        velocity.y = sin(radians) * currentSpeed;
        
        // Move
        position = position + velocity;
        
        // Check collisions with map
        if (checkTileCollision(map)) {
            position = oldPosition;
            rotation += 180.0f; // Turn around if we hit something
            if (rotation >= 360.0f) rotation -= 360.0f;
        }
        
        // Check collisions with other entities
        for (const auto& entity : entities) {
            if (entity.get() != this && checkCollision(*entity)) {
                position = oldPosition;
                currentSpeed *= -0.5f; // Bounce back a bit
                break;
            }
        }
    }
    
    void accelerate() {
        currentSpeed += acceleration;
        if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
    }
    
    void brake() {
        currentSpeed -= deceleration * 2;
        if (currentSpeed < -maxSpeed / 2) currentSpeed = -maxSpeed / 2;
    }
    
    void decelerate() {
        if (currentSpeed > 0) {
            currentSpeed -= deceleration;
            if (currentSpeed < 0) currentSpeed = 0;
        } else if (currentSpeed < 0) {
            currentSpeed += deceleration;
            if (currentSpeed > 0) currentSpeed = 0;
        }
    }
    
    void turnLeft() {
        rotation -= turnSpeed;
        if (rotation < 0) rotation += 360.0f;
    }
    
    void turnRight() {
        rotation += turnSpeed;
        if (rotation >= 360.0f) rotation -= 360.0f;
    }
    
    // Getters and setters
    bool isOccupied() const { return occupied; }
    void setOccupied(bool occ) { occupied = occ; }
    float getCurrentSpeed() const { return currentSpeed; }
};

// Character class (player and NPCs)
class Character : public Entity {
private:
    std::shared_ptr<Vehicle> currentVehicle;
    bool isPlayer;
    
public:
    Character(SDL_Texture* tex, float x, float y, bool player) :
        Entity(tex, x, y, 32, 32, player ? ENTITY_PLAYER : ENTITY_NPC),
        currentVehicle(nullptr), isPlayer(player) {
        speed = PLAYER_SPEED;
    }
    
    void update(float deltaTime, const std::vector<std::shared_ptr<Entity>>& entities, const std::vector<std::vector<TileType>>& map) override {
        if (currentVehicle) {
            // If in a vehicle, position matches the vehicle
            position = currentVehicle->getPosition();
            rotation = currentVehicle->getRotation();
            return;
        }
        
        // Save old position for collision resolution
        Vector2 oldPosition = position;
        
        // Move character
        position = position + velocity * speed;
        
        // Check collisions with map
        if (checkTileCollision(map)) {
            position = oldPosition;
        }
        
        // Check collisions with other entities
        for (const auto& entity : entities) {
            if (entity.get() != this && entity->getType() != ENTITY_PLAYER && 
                entity->getType() != ENTITY_NPC && checkCollision(*entity)) {
                position = oldPosition;
                break;
            }
        }
        
        // If not player, apply some AI behavior
        if (!isPlayer) {
            // Simple AI: wander around
            static std::mt19937 rng(std::time(nullptr));
            std::uniform_int_distribution<int> moveDist(0, 100);
            
            if (moveDist(rng) < 2) {
                std::uniform_int_distribution<int> dirDist(0, 3);
                int dir = dirDist(rng);
                
                if (dir == 0) velocity = Vector2(1, 0);
                else if (dir == 1) velocity = Vector2(-1, 0);
                else if (dir == 2) velocity = Vector2(0, 1);
                else velocity = Vector2(0, -1);
            }
        }
    }
    
    void enterVehicle(std::shared_ptr<Vehicle> vehicle) {
        if (!vehicle || vehicle->isOccupied()) return;
        
        currentVehicle = vehicle;
        vehicle->setOccupied(true);
    }
    
    void exitVehicle() {
        if (!currentVehicle) return;
        
        // Place character next to vehicle
        float radians = (currentVehicle->getRotation() + 90) * M_PI / 180.0f;
        position.x = currentVehicle->getPosition().x + cos(radians) * 50;
        position.y = currentVehicle->getPosition().y + sin(radians) * 50;
        
        currentVehicle->setOccupied(false);
        currentVehicle = nullptr;
    }
    
    void setVelocity(const Vector2& vel) {
        velocity = vel.normalized();
    }
    
    bool isInVehicle() const { return currentVehicle != nullptr; }
    std::shared_ptr<Vehicle> getVehicle() const { return currentVehicle; }
};

// World/map generation
class World {
private:
    std::vector<std::vector<TileType>> map;
    
public:
    World() : map(MAP_HEIGHT, std::vector<TileType>(MAP_WIDTH, TILE_GRASS)) {
        generateMap();
    }
    
    void generateMap() {
        // Simple city grid layout
        
        // First, fill with grass
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                map[y][x] = TILE_GRASS;
            }
        }
        
        // Create a grid of roads
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (x % 8 == 0 || y % 8 == 0) {
                    map[y][x] = TILE_ROAD;
                }
            }
        }
        
        // Add some buildings
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (map[y][x] == TILE_GRASS) {
                    if ((y > 1 && y < MAP_HEIGHT - 2) && (x > 1 && x < MAP_WIDTH - 2)) {
                        if ((y % 8 > 1 && y % 8 < 7) && (x % 8 > 1 && x % 8 < 7)) {
                            // Random building size
                            std::mt19937 rng(y * MAP_WIDTH + x);
                            std::uniform_int_distribution<int> buildingDist(0, 10);
                            
                            if (buildingDist(rng) < 7) {
                                map[y][x] = TILE_BUILDING;
                            }
                        }
                    }
                }
            }
        }
        
        // Add some water
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (x < 5 && y > MAP_HEIGHT - 15) {
                    map[y][x] = TILE_WATER;
                }
            }
        }
    }
    
    void render(SDL_Renderer* renderer, SDL_Texture* tileset, const SDL_Rect& camera) {
        int startX = camera.x / TILE_SIZE;
        int startY = camera.y / TILE_SIZE;
        int endX = startX + (SCREEN_WIDTH / TILE_SIZE) + 1;
        int endY = startY + (SCREEN_HEIGHT / TILE_SIZE) + 1;
        
        // Clamp to map boundaries
        startX = std::max(0, startX);
        startY = std::max(0, startY);
        endX = std::min(MAP_WIDTH, endX);
        endY = std::min(MAP_HEIGHT, endY);
        
        // Render visible tiles
        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                SDL_Rect srcRect;
                switch (map[y][x]) {
                    case TILE_ROAD:
                        srcRect = {0, 0, TILE_SIZE, TILE_SIZE};
                        break;
                    case TILE_GRASS:
                        srcRect = {TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
                        break;
                    case TILE_BUILDING:
                        srcRect = {TILE_SIZE * 2, 0, TILE_SIZE, TILE_SIZE};
                        break;
                    case TILE_WATER:
                        srcRect = {TILE_SIZE * 3, 0, TILE_SIZE, TILE_SIZE};
                        break;
                }
                
                SDL_Rect destRect = {
                    (int)(x * TILE_SIZE - camera.x),
                    (int)(y * TILE_SIZE - camera.y),
                    TILE_SIZE,
                    TILE_SIZE
                };
                
                SDL_RenderCopy(renderer, tileset, &srcRect, &destRect);
            }
        }
    }
    
    const std::vector<std::vector<TileType>>& getMap() const {
        return map;
    }
};

// Main game class
class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    GameState state;
    
    // Textures
    SDL_Texture* playerTexture;
    SDL_Texture* npcTexture;
    SDL_Texture* carTexture;
    SDL_Texture* policeCarTexture;
    SDL_Texture* tilesetTexture;
    
    // Game objects
    std::shared_ptr<Character> player;
    std::vector<std::shared_ptr<Entity>> entities;
    World world;
    Camera camera;
    
    // Input handling
    const Uint8* keyboardState;
    
    // Timing
    Uint32 lastFrameTime;
    const int FPS = 60;
    const int FRAME_DELAY = 1000 / FPS;
    
public:
    Game() : window(nullptr), renderer(nullptr), running(false),
             state(STATE_WALKING), keyboardState(nullptr), lastFrameTime(0) {}
    
    ~Game() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_image
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return false;
        }
        
        // Create window
        window = SDL_CreateWindow("GTA Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Get keyboard state
        keyboardState = SDL_GetKeyboardState(nullptr);
        
        // Load textures
        playerTexture = loadTexture("player.png");
        npcTexture = loadTexture("npc.png");
        carTexture = loadTexture("car.png");
        policeCarTexture = loadTexture("police_car.png");
        tilesetTexture = loadTexture("tileset.png");
        
        if (!playerTexture || !npcTexture || !carTexture || !policeCarTexture || !tilesetTexture) {
            std::cerr << "Failed to load textures!" << std::endl;
            return false;
        }
        
        // Initialize game objects
        player = std::make_shared<Character>(playerTexture, MAP_WIDTH * TILE_SIZE / 2, MAP_HEIGHT * TILE_SIZE / 2, true);
        entities.push_back(player);
        
        // Add some NPCs
        for (int i = 0; i < 20; i++) {
            int x = (rand() % (MAP_WIDTH - 4) + 2) * TILE_SIZE;
            int y = (rand() % (MAP_HEIGHT - 4) + 2) * TILE_SIZE;
            entities.push_back(std::make_shared<Character>(npcTexture, x, y, false));
        }
        
        // Add some vehicles
        for (int i = 0; i < 15; i++) {
            int x = (rand() % (MAP_WIDTH - 4) + 2) * TILE_SIZE;
            int y = (rand() % (MAP_HEIGHT - 4) + 2) * TILE_SIZE;
            entities.push_back(std::make_shared<Vehicle>(carTexture, x, y, 48, 24, ENTITY_CAR));
        }
        
        // Add some police cars
        for (int i = 0; i < 5; i++) {
            int x = (rand() % (MAP_WIDTH - 4) + 2) * TILE_SIZE;
            int y = (rand() % (MAP_HEIGHT - 4) + 2) * TILE_SIZE;
            entities.push_back(std::make_shared<Vehicle>(policeCarTexture, x, y, 48, 24, ENTITY_POLICE_CAR));
        }
        
        running = true;
        lastFrameTime = SDL_GetTicks();
        
        return true;
    }
    
    void run() {
        while (running) {
            Uint32 frameStart = SDL_GetTicks();
            
            handleEvents();
            update();
            render();
            
            // Frame rate control
            Uint32 frameTime = SDL_GetTicks() - frameStart;
            if (frameTime < FRAME_DELAY) {
                SDL_Delay(FRAME_DELAY - frameTime);
            }
        }
    }
    
private:
    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_f) {
                    // Enter/exit vehicle
                    if (state == STATE_WALKING) {
                        // Try to enter a vehicle
                        for (const auto& entity : entities) {
                            if ((entity->getType() == ENTITY_CAR || entity->getType() == ENTITY_POLICE_CAR) &&
                                dynamic_cast<Vehicle*>(entity.get()) &&
                                !dynamic_cast<Vehicle*>(entity.get())->isOccupied() &&
                                distance(player->getPosition().x, player->getPosition().y,
                                         entity->getPosition().x, entity->getPosition().y) < 60) {
                                
                                player->enterVehicle(std::dynamic_pointer_cast<Vehicle>(entity));
                                state = STATE_DRIVING;
                                break;
                            }
                        }
                    } else if (state == STATE_DRIVING) {
                        // Exit vehicle
                        player->exitVehicle();
                        state = STATE_WALKING;
                    }
                }
            }
        }
        
        // Handle keyboard input
        if (state == STATE_WALKING) {
            // Walking controls
            Vector2 movement(0, 0);
            
            if (keyboardState[SDL_SCANCODE_W] || keyboardState[SDL_SCANCODE_UP]) {
                movement.y = -1;
            }
            if (keyboardState[SDL_SCANCODE_S] || keyboardState[SDL_SCANCODE_DOWN]) {
                movement.y = 1;
            }
            if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
                movement.x = -1;
            }
            if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
                movement.x = 1;
            }
            
            player->setVelocity(movement);
        } else if (state == STATE_DRIVING) {
            // Driving controls
            auto vehicle = player->getVehicle();
            if (vehicle) {
                if (keyboardState[SDL_SCANCODE_W] || keyboardState[SDL_SCANCODE_UP]) {
                    vehicle->accelerate();
                } else if (keyboardState[SDL_SCANCODE_S] || keyboardState[SDL_SCANCODE_DOWN]) {
                    vehicle->brake();
                } else {
                    vehicle->decelerate();
                }
                
                if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
                    vehicle->turnLeft();
                }
                if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
                    vehicle->turnRight();
                }
            }
        }
    }
    
    void update() {
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentTime;
        
        // Cap delta time to prevent large jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        // Use a fixed time step for more consistent simulation
        const float fixedDeltaTime = 1.0f / FPS;
        
        // Update all entities with the fixed time step
        for (auto& entity : entities) {
            entity->update(fixedDeltaTime, entities, world.getMap());
        }
    }
    
    void render() {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Get camera viewport centered on player
        SDL_Rect camera = this->camera.getViewport(player->getPosition());
        
        // Render world
        world.render(renderer, tilesetTexture, camera);
        
        // Sort entities by Y position for proper depth
        std::sort(entities.begin(), entities.end(), [](const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b) {
            return a->getPosition().y < b->getPosition().y;
        });
        
        // Render entities
        for (auto& entity : entities) {
            entity->render(renderer, camera);
        }
        
        // Display HUD
        renderHUD();
        
        // Present renderer
        SDL_RenderPresent(renderer);
    }
    
    void renderHUD() {
        // This is a simple placeholder for HUD rendering
        // In a full game, you'd want to display health, money, wanted level, minimap, etc.
        
        // For now, just show the game state
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_Rect statusRect = {10, 10, 200, 30};
        SDL_RenderFillRect(renderer, &statusRect);
        
        // Actual text rendering would require SDL_ttf, which isn't included in this example
    }
    
    SDL_Texture* loadTexture(const std::string& path) {
        // In a real implementation, you'd load actual image files
        // For this example, we'll create colored rectangles as placeholders
        
        SDL_Surface* surface = SDL_CreateRGBSurface(0, TILE_SIZE, TILE_SIZE, 32, 0, 0, 0, 0);
        
        if (path == "player.png") {
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 255)); // Blue
        } else if (path == "npc.png") {
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 0)); // Red
        } else if (path == "car.png") {
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 255, 0)); // Green
        } else if (path == "police_car.png") {
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0)); // Black with blue top
            SDL_Rect blueRect = {0, 0, TILE_SIZE, TILE_SIZE / 2};
            SDL_FillRect(surface, &blueRect, SDL_MapRGB(surface->format, 0, 0, 255));
        } else if (path == "tileset.png") {
            // Create a tileset with 4 tiles: road, grass, building, water
            surface = SDL_CreateRGBSurface(0, TILE_SIZE * 4, TILE_SIZE, 32, 0, 0, 0, 0);
            
            // Road (gray)
            SDL_Rect roadRect = {0, 0, TILE_SIZE, TILE_SIZE};
            SDL_FillRect(surface, &roadRect, SDL_MapRGB(surface->format, 100, 100, 100));
            
            // Grass (green)
            SDL_Rect grassRect = {TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
            SDL_FillRect(surface, &grassRect, SDL_MapRGB(surface->format, 0, 128, 0));
            
            // Building (brown)
            SDL_Rect buildingRect = {TILE_SIZE * 2, 0, TILE_SIZE, TILE_SIZE};
            SDL_FillRect(surface, &buildingRect, SDL_MapRGB(surface->format, 139, 69, 19));
            
            // Water (blue)
            SDL_Rect waterRect = {TILE_SIZE * 3, 0, TILE_SIZE, TILE_SIZE};
            SDL_FillRect(surface, &waterRect, SDL_MapRGB(surface->format, 30, 144, 255));
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }
    
    void cleanup() {
        // Clean up textures
        if (playerTexture) SDL_DestroyTexture(playerTexture);
        if (npcTexture) SDL_DestroyTexture(npcTexture);
        if (carTexture) SDL_DestroyTexture(carTexture);
        if (policeCarTexture) SDL_DestroyTexture(policeCarTexture);
        if (tilesetTexture) SDL_DestroyTexture(tilesetTexture);
        
        // Clear entities
        entities.clear();
        
        // Clean up SDL
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
    }
};

// Main function
int main(int argc, char* argv[]) {
    Game game;
    
    if (!game.initialize()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }
    
    game.run();
    
    return 0;
}
