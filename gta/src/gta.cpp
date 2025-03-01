#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float PLAYER_SPEED = 3.0f;
const float VEHICLE_SPEED = 5.0f;
const float VEHICLE_ROTATION_SPEED = 2.5f;

// Game object base class
class GameObject {
protected:
    float x, y;
    float width, height;
    float rotation; // in degrees
    SDL_Texture* texture;
    bool collidable;

public:
    GameObject(float x, float y, float w, float h, SDL_Texture* tex, bool collidable = true) 
    : x(x), y(y), width(w), height(h), texture(tex), rotation(0.0f), collidable(collidable) {}
    
    virtual ~GameObject() {}
    
    virtual void update(float deltaTime) = 0;
    
    virtual void render(SDL_Renderer* renderer, int cameraX, int cameraY) {
        SDL_Rect destRect = {
            static_cast<int>(x - cameraX),
            static_cast<int>(y - cameraY),
            static_cast<int>(width),
            static_cast<int>(height)
        };
        
        SDL_RenderCopyEx(
            renderer,
            texture,
            NULL,
            &destRect,
            rotation,
            NULL,
            SDL_FLIP_NONE
        );
    }
    
    SDL_Rect getCollisionBox() const {
        return {
            static_cast<int>(x),
            static_cast<int>(y),
            static_cast<int>(width),
            static_cast<int>(height)
        };
    }
    
    bool isCollidable() const { return collidable; }
    float getX() const { return x; }
    float getY() const { return y; }
    float getRotation() const { return rotation; }
    void setPosition(float newX, float newY) { x = newX; y = newY; }
};

// Building/obstacle class
class Building : public GameObject {
public:
    Building(float x, float y, float w, float h, SDL_Texture* tex) 
    : GameObject(x, y, w, h, tex) {}
    
    void update(float deltaTime) override {}
};

// Vehicle class
class Vehicle : public GameObject {
private:
    float speed;
    float maxSpeed;
    bool occupied;
    
public:
    Vehicle(float x, float y, SDL_Texture* tex) 
    : GameObject(x, y, 64, 32, tex), speed(0.0f), maxSpeed(VEHICLE_SPEED), occupied(false) {}
    
    void update(float deltaTime) override {
        if (occupied && speed != 0) {
            float radians = rotation * M_PI / 180.0f;
            x += std::cos(radians) * speed * deltaTime;
            y += std::sin(radians) * speed * deltaTime;
        }
    }
    
    void accelerate(float amount) {
        speed += amount;
        if (speed > maxSpeed) speed = maxSpeed;
        if (speed < -maxSpeed/2) speed = -maxSpeed/2;
    }
    
    void brake() {
        if (speed > 0) speed -= 0.1f;
        else if (speed < 0) speed += 0.1f;
        
        if (std::abs(speed) < 0.1f) speed = 0;
    }
    
    void turn(float amount) {
        rotation += amount * std::min(1.0f, std::abs(speed) / maxSpeed);
    }
    
    void setOccupied(bool isOccupied) { occupied = isOccupied; }
    bool isOccupied() const { return occupied; }
    float getSpeed() const { return speed; }
};

// Player character class
class Player : public GameObject {
private:
    float speedX, speedY;
    Vehicle* currentVehicle;
    bool inVehicle;
    
public:
    Player(float x, float y, SDL_Texture* tex) 
    : GameObject(x, y, 32, 32, tex), speedX(0), speedY(0), currentVehicle(nullptr), inVehicle(false) {}
    
    void update(float deltaTime) override {
        if (!inVehicle) {
            x += speedX * deltaTime;
            y += speedY * deltaTime;
        }
    }
    
    void setSpeed(float x, float y) {
        if (!inVehicle) {
            speedX = x;
            speedY = y;
            
            // Set rotation based on movement direction
            if (speedX != 0 || speedY != 0) {
                rotation = std::atan2(speedY, speedX) * 180.0f / M_PI;
            }
        }
    }
    
    bool enterVehicle(Vehicle* vehicle) {
        if (!inVehicle && vehicle && !vehicle->isOccupied()) {
            currentVehicle = vehicle;
            inVehicle = true;
            vehicle->setOccupied(true);
            return true;
        }
        return false;
    }
    
    void exitVehicle() {
        if (inVehicle && currentVehicle) {
            // Position player next to vehicle
            float radians = currentVehicle->getRotation() * M_PI / 180.0f;
            x = currentVehicle->getX() + std::cos(radians + M_PI/2) * 40;
            y = currentVehicle->getY() + std::sin(radians + M_PI/2) * 40;
            
            currentVehicle->setOccupied(false);
            inVehicle = false;
            currentVehicle = nullptr;
        }
    }
    
    bool isInVehicle() const { return inVehicle; }
    Vehicle* getCurrentVehicle() const { return currentVehicle; }
};

// Game class
class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool isRunning;
    Uint32 lastTime;
    
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Vehicle>> vehicles;
    std::vector<std::unique_ptr<Building>> buildings;
    
    SDL_Texture* playerTexture;
    SDL_Texture* vehicleTexture;
    SDL_Texture* buildingTexture;
    SDL_Texture* roadTexture;
    
    int cameraX, cameraY;
    
    bool checkCollision(const SDL_Rect& a, const SDL_Rect& b) {
        return (a.x < b.x + b.w &&
                a.x + a.w > b.x &&
                a.y < b.y + b.h &&
                a.y + a.h > b.y);
    }
    
public:
    Game() : window(nullptr), renderer(nullptr), isRunning(false), lastTime(0), cameraX(0), cameraY(0) {}
    
    ~Game() {
        cleanup();
    }
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
            std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow("GTA Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                 SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Load textures - In a real game, you'd have proper textures
        SDL_Surface* tempSurface = SDL_CreateRGBSurface(0, 32, 32, 32, 0, 0, 0, 0);
        SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 255, 0, 0));
        playerTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);
        
        tempSurface = SDL_CreateRGBSurface(0, 64, 32, 32, 0, 0, 0, 0);
        SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 0, 0, 255));
        vehicleTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);
        
        tempSurface = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);
        SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 100, 100, 100));
        buildingTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);
        
        tempSurface = SDL_CreateRGBSurface(0, 800, 600, 32, 0, 0, 0, 0);
        SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 50, 50, 50));
        roadTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);
        
        // Create player
        player = std::make_unique<Player>(400, 300, playerTexture);
        
        // Create vehicles
        vehicles.push_back(std::make_unique<Vehicle>(200, 200, vehicleTexture));
        vehicles.push_back(std::make_unique<Vehicle>(500, 400, vehicleTexture));
        
        // Create buildings/obstacles
        buildings.push_back(std::make_unique<Building>(100, 100, 100, 100, buildingTexture));
        buildings.push_back(std::make_unique<Building>(600, 200, 100, 100, buildingTexture));
        buildings.push_back(std::make_unique<Building>(300, 500, 100, 100, buildingTexture));
        
        isRunning = true;
        lastTime = SDL_GetTicks();
        
        return true;
    }
    
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                isRunning = false;
            } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
                if (e.key.keysym.sym == SDLK_e) {
                    // Toggle vehicle entry/exit
                    if (player->isInVehicle()) {
                        player->exitVehicle();
                    } else {
                        // Find nearest vehicle
                        float minDist = 100.0f; // Maximum distance to enter
                        Vehicle* nearestVehicle = nullptr;
                        
                        for (auto& vehicle : vehicles) {
                            float dx = vehicle->getX() - player->getX();
                            float dy = vehicle->getY() - player->getY();
                            float dist = std::sqrt(dx*dx + dy*dy);
                            
                            if (dist < minDist && !vehicle->isOccupied()) {
                                minDist = dist;
                                nearestVehicle = vehicle.get();
                            }
                        }
                        
                        if (nearestVehicle) {
                            player->enterVehicle(nearestVehicle);
                        }
                    }
                }
            }
        }
        
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        
        if (player->isInVehicle()) {
            // Vehicle controls
            Vehicle* vehicle = player->getCurrentVehicle();
            
            if (keystate[SDL_SCANCODE_W]) {
                vehicle->accelerate(0.1f);
            } else if (keystate[SDL_SCANCODE_S]) {
                vehicle->accelerate(-0.1f);
            } else {
                vehicle->brake();
            }
            
            if (keystate[SDL_SCANCODE_A]) {
                vehicle->turn(-VEHICLE_ROTATION_SPEED);
            } else if (keystate[SDL_SCANCODE_D]) {
                vehicle->turn(VEHICLE_ROTATION_SPEED);
            }
        } else {
            // Player controls
            float speedX = 0, speedY = 0;
            
            if (keystate[SDL_SCANCODE_W]) speedY = -PLAYER_SPEED;
            if (keystate[SDL_SCANCODE_S]) speedY = PLAYER_SPEED;
            if (keystate[SDL_SCANCODE_A]) speedX = -PLAYER_SPEED;
            if (keystate[SDL_SCANCODE_D]) speedX = PLAYER_SPEED;
            
            player->setSpeed(speedX, speedY);
        }
    }
    
    void update() {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 10.0f; // Scale for smoother movement
        lastTime = currentTime;
        
        // Update player
        player->update(deltaTime);
        
        // Update vehicles
        for (auto& vehicle : vehicles) {
            vehicle->update(deltaTime);
        }
        
        // Handle collisions between player and buildings
        if (!player->isInVehicle()) {
            SDL_Rect playerRect = player->getCollisionBox();
            
            for (auto& building : buildings) {
                if (checkCollision(playerRect, building->getCollisionBox())) {
                    // Simple collision response - push player back
                    float pushX = player->getX();
                    float pushY = player->getY();
                    
                    // Determine push direction
                    SDL_Rect buildingRect = building->getCollisionBox();
                    float centerPlayerX = playerRect.x + playerRect.w / 2;
                    float centerPlayerY = playerRect.y + playerRect.h / 2;
                    float centerBuildingX = buildingRect.x + buildingRect.w / 2;
                    float centerBuildingY = buildingRect.y + buildingRect.h / 2;
                    
                    float dx = centerPlayerX - centerBuildingX;
                    float dy = centerPlayerY - centerBuildingY;
                    
                    // Simple normalization
                    float length = std::sqrt(dx*dx + dy*dy);
                    if (length > 0) {
                        dx /= length;
                        dy /= length;
                    }
                    
                    pushX += dx * 5.0f;
                    pushY += dy * 5.0f;
                    
                    player->setPosition(pushX, pushY);
                }
            }
        }
        
        // Update camera to follow player
        if (player->isInVehicle()) {
            cameraX = static_cast<int>(player->getCurrentVehicle()->getX() - SCREEN_WIDTH / 2);
            cameraY = static_cast<int>(player->getCurrentVehicle()->getY() - SCREEN_HEIGHT / 2);
        } else {
            cameraX = static_cast<int>(player->getX() - SCREEN_WIDTH / 2);
            cameraY = static_cast<int>(player->getY() - SCREEN_HEIGHT / 2);
        }
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw background/road
        SDL_Rect roadRect = {-cameraX, -cameraY, 2000, 2000}; // Large area
        SDL_RenderCopy(renderer, roadTexture, NULL, &roadRect);
        
        // Draw buildings
        for (auto& building : buildings) {
            building->render(renderer, cameraX, cameraY);
        }
        
        // Draw vehicles
        for (auto& vehicle : vehicles) {
            vehicle->render(renderer, cameraX, cameraY);
        }
        
        // Draw player if not in vehicle
        if (!player->isInVehicle()) {
            player->render(renderer, cameraX, cameraY);
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void run() {
        while (isRunning) {
            handleEvents();
            update();
            render();
            SDL_Delay(16); // ~60 FPS
        }
    }
    
    void cleanup() {
        if (playerTexture) SDL_DestroyTexture(playerTexture);
        if (vehicleTexture) SDL_DestroyTexture(vehicleTexture);
        if (buildingTexture) SDL_DestroyTexture(buildingTexture);
        if (roadTexture) SDL_DestroyTexture(roadTexture);
        
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        
        IMG_Quit();
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    Game game;
    
    if (!game.initialize()) {
        std::cout << "Failed to initialize game!" << std::endl;
        return -1;
    }
    
    game.run();
    
    return 0;
}
