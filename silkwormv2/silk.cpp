// Silkworm Clone - SDL2 Implementation
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>

// Game constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SCROLL_SPEED = 2;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;
const float ENEMY_SPAWN_RATE = 0.02f;
const int HELICOPTER_HEIGHT = 60;
const int HELICOPTER_WIDTH = 100;
const int JEEP_HEIGHT = 40;
const int JEEP_WIDTH = 80;
const int ENEMY_WIDTH = 60;
const int ENEMY_HEIGHT = 40;
const int BULLET_WIDTH = 8;
const int BULLET_HEIGHT = 4;
const int MAX_BULLETS = 30;

// Forward declarations
class GameObject;
class Player;
class Enemy;
class Bullet;
class Explosion;
class GameWorld;

// Enums
enum class EntityType {
    HELICOPTER,
    JEEP,
    ENEMY_GROUND,
    ENEMY_AIR,
    ENEMY_TURRET,
    BULLET_PLAYER,
    BULLET_ENEMY,
    EXPLOSION,
    BACKGROUND
};

enum class PlayerType {
    HELICOPTER,
    JEEP
};

// Basic game object
class GameObject {
public:
    GameObject(float x, float y, int width, int height, EntityType type) 
        : x(x), y(y), width(width), height(height), type(type), isActive(true) {}
    
    virtual ~GameObject() = default;
    virtual void update() = 0;
    virtual void render(SDL_Renderer* renderer) = 0;
    
    bool checkCollision(const GameObject& other) const {
        return (x < other.x + other.width &&
                x + width > other.x &&
                y < other.y + other.height &&
                y + height > other.y);
    }
    
    bool isAlive() const { return isActive; }
    void destroy() { isActive = false; }
    
    EntityType getType() const { return type; }
    
    float getX() const { return x; }
    float getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

protected:
    float x, y;
    int width, height;
    EntityType type;
    bool isActive;
};

// Player class (Helicopter or Jeep)
class Player : public GameObject {
public:
    Player(float x, float y, PlayerType playerType) 
        : GameObject(x, y, 
                    playerType == PlayerType::HELICOPTER ? HELICOPTER_WIDTH : JEEP_WIDTH, 
                    playerType == PlayerType::HELICOPTER ? HELICOPTER_HEIGHT : JEEP_HEIGHT, 
                    playerType == PlayerType::HELICOPTER ? EntityType::HELICOPTER : EntityType::JEEP),
          playerType(playerType), score(0), lives(3), bulletCooldown(0) {}
    
    void update() override {
        if (bulletCooldown > 0) {
            bulletCooldown--;
        }
        
        // Boundaries check
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > SCREEN_WIDTH - width) x = SCREEN_WIDTH - width;
        if (playerType == PlayerType::HELICOPTER) {
            if (y > SCREEN_HEIGHT - height) y = SCREEN_HEIGHT - height;
        } else {
            // Jeep is restricted to the ground level
            if (y < SCREEN_HEIGHT - height - 50) y = SCREEN_HEIGHT - height - 50;
            if (y > SCREEN_HEIGHT - height) y = SCREEN_HEIGHT - height;
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), width, height};
        
        // Different colors for helicopter and jeep
        if (playerType == PlayerType::HELICOPTER) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for helicopter
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for jeep
        }
        
        SDL_RenderFillRect(renderer, &rect);
    }
    
    void moveUp() {
        y -= PLAYER_SPEED;
    }
    
    void moveDown() {
        y += PLAYER_SPEED;
    }
    
    void moveLeft() {
        x -= PLAYER_SPEED;
    }
    
    void moveRight() {
        x += PLAYER_SPEED;
    }
    
    bool canShoot() const {
        return bulletCooldown == 0;
    }
    
    void shoot() {
        bulletCooldown = 10; // Cooldown period
    }
    
    void increaseScore(int points) {
        score += points;
    }
    
    int getScore() const { return score; }
    int getLives() const { return lives; }
    void loseLife() { lives--; }
    PlayerType getPlayerType() const { return playerType; }

private:
    PlayerType playerType;
    int score;
    int lives;
    int bulletCooldown;
};

// Bullet class
class Bullet : public GameObject {
public:
    Bullet(float x, float y, float vx, float vy, bool isPlayerBullet)
        : GameObject(x, y, BULLET_WIDTH, BULLET_HEIGHT, 
                    isPlayerBullet ? EntityType::BULLET_PLAYER : EntityType::BULLET_ENEMY),
          vx(vx), vy(vy) {}
    
    void update() override {
        x += vx;
        y += vy;
        
        // Destroy if out of bounds
        if (x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT) {
            destroy();
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), width, height};
        
        if (type == EntityType::BULLET_PLAYER) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow for player bullets
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for enemy bullets
        }
        
        SDL_RenderFillRect(renderer, &rect);
    }

private:
    float vx, vy; // Bullet velocity
};

// Enemy class
class Enemy : public GameObject {
public:
    Enemy(float x, float y, EntityType enemyType, float speed) 
        : GameObject(x, y, ENEMY_WIDTH, ENEMY_HEIGHT, enemyType),
          vx(-speed), shootTimer(rand() % 100) {}
    
    void update() override {
        x += vx;
        
        // Simple AI based on enemy type
        if (type == EntityType::ENEMY_AIR) {
            // Flying enemies move in a sine wave pattern
            y += std::sin(x * 0.05f) * 2.0f;
        } else if (type == EntityType::ENEMY_TURRET) {
            // Turrets don't move horizontally
            x += SCROLL_SPEED; // Counter scrolling
        }
        
        // Destroy if out of bounds
        if (x < -width) {
            destroy();
        }
        
        // Decrease shoot timer
        if (shootTimer > 0) {
            shootTimer--;
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), width, height};
        
        if (type == EntityType::ENEMY_AIR) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // Purple for air enemies
        } else if (type == EntityType::ENEMY_GROUND) {
            SDL_SetRenderDrawColor(renderer, 128, 128, 0, 255); // Olive for ground enemies
        } else { // ENEMY_TURRET
            SDL_SetRenderDrawColor(renderer, 255, 128, 0, 255); // Orange for turrets
        }
        
        SDL_RenderFillRect(renderer, &rect);
    }
    
    bool canShoot() const {
        return shootTimer == 0;
    }
    
    void shoot() {
        shootTimer = 50 + (rand() % 50); // Random cooldown
    }

private:
    float vx;
    int shootTimer;
};

// Explosion class
class Explosion : public GameObject {
public:
    Explosion(float x, float y) 
        : GameObject(x, y, 40, 40, EntityType::EXPLOSION), 
          lifetime(20), animation(0) {}
    
    void update() override {
        animation++;
        if (animation >= lifetime) {
            destroy();
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        int size = 40 * (1.0f - static_cast<float>(animation) / lifetime);
        SDL_Rect rect = {static_cast<int>(x - size/2), static_cast<int>(y - size/2), size, size};
        
        // Change color based on explosion stage
        Uint8 r = 255;
        Uint8 g = static_cast<Uint8>(255 * (1.0f - static_cast<float>(animation) / lifetime));
        Uint8 b = 0;
        
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

private:
    int lifetime;
    int animation;
};

// Background parallax layer
class BackgroundLayer : public GameObject {
public:
    BackgroundLayer(float x, float y, int width, int height, float scrollSpeed, Uint8 r, Uint8 g, Uint8 b)
        : GameObject(x, y, width, height, EntityType::BACKGROUND),
          scrollSpeed(scrollSpeed), r(r), g(g), b(b), duplicate_x(x + width) {}
    
    void update() override {
        x -= scrollSpeed;
        duplicate_x -= scrollSpeed;
        
        // Wrap around when off-screen
        if (x <= -width) {
            x = duplicate_x + width;
        }
        if (duplicate_x <= -width) {
            duplicate_x = x + width;
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        SDL_Rect rect1 = {static_cast<int>(x), static_cast<int>(y), width, height};
        SDL_Rect rect2 = {static_cast<int>(duplicate_x), static_cast<int>(y), width, height};
        
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &rect1);
        SDL_RenderFillRect(renderer, &rect2);
    }

private:
    float scrollSpeed;
    Uint8 r, g, b;
    float duplicate_x; // For seamless scrolling
};

// Game world that manages all game objects
class GameWorld {
public:
    GameWorld() : gameOver(false), levelDistance(0), rng(std::random_device()()) {
        // Create background layers
        backgrounds.push_back(std::make_unique<BackgroundLayer>(
            0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.5f, 100, 100, 200)); // Sky
        backgrounds.push_back(std::make_unique<BackgroundLayer>(
            0, SCREEN_HEIGHT - 200, SCREEN_WIDTH, 200, 1.0f, 50, 100, 50)); // Hills
        backgrounds.push_back(std::make_unique<BackgroundLayer>(
            0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, SCROLL_SPEED, 139, 69, 19)); // Ground
    }
    
    void init(PlayerType playerType) {
        // Create player based on selected type
        if (playerType == PlayerType::HELICOPTER) {
            player = std::make_unique<Player>(100, SCREEN_HEIGHT / 2 - HELICOPTER_HEIGHT / 2, PlayerType::HELICOPTER);
        } else {
            player = std::make_unique<Player>(100, SCREEN_HEIGHT - JEEP_HEIGHT - 50, PlayerType::JEEP);
        }
    }
    
    void update() {
        if (gameOver) return;
        
        // Update level progression
        levelDistance += SCROLL_SPEED;
        
        // Update all objects
        for (auto& bg : backgrounds) {
            bg->update();
        }
        
        if (player) {
            player->update();
        }
        
        for (auto& bullet : bullets) {
            bullet->update();
        }
        
        for (auto& enemy : enemies) {
            enemy->update();
            
            // Enemy shooting logic
            if (enemy->canShoot() && player) {
                enemy->shoot();
                float dx = player->getX() - enemy->getX();
                float dy = player->getY() - enemy->getY();
                float dist = std::sqrt(dx*dx + dy*dy);
                if (dist > 0) {
                    dx /= dist;
                    dy /= dist;
                }
                addBullet(enemy->getX(), enemy->getY(), dx * BULLET_SPEED * 0.5f, dy * BULLET_SPEED * 0.5f, false);
            }
        }
        
        for (auto& explosion : explosions) {
            explosion->update();
        }
        
        // Spawn enemies randomly
        spawnEnemies();
        
        // Check for collisions
        checkCollisions();
        
        // Clean up destroyed objects
        cleanup();
        
        // Check game over condition
        if (player && player->getLives() <= 0) {
            gameOver = true;
        }
    }
    
    void render(SDL_Renderer* renderer) {
        // Render all objects
        for (auto& bg : backgrounds) {
            bg->render(renderer);
        }
        
        for (auto& enemy : enemies) {
            enemy->render(renderer);
        }
        
        if (player) {
            player->render(renderer);
        }
        
        for (auto& bullet : bullets) {
            bullet->render(renderer);
        }
        
        for (auto& explosion : explosions) {
            explosion->render(renderer);
        }
        
        // Render HUD
        renderHUD(renderer);
    }
    
    void handleInput(const Uint8* keystate) {
        if (!player || gameOver) return;
        
        if (keystate[SDL_SCANCODE_UP]) {
            player->moveUp();
        }
        if (keystate[SDL_SCANCODE_DOWN]) {
            player->moveDown();
        }
        if (keystate[SDL_SCANCODE_LEFT]) {
            player->moveLeft();
        }
        if (keystate[SDL_SCANCODE_RIGHT]) {
            player->moveRight();
        }
        if (keystate[SDL_SCANCODE_SPACE] && player->canShoot()) {
            player->shoot();
            
            // Different bullet patterns based on player type
            float px = player->getX() + player->getWidth();
            float py = player->getY() + player->getHeight() / 2;
            
            if (player->getPlayerType() == PlayerType::HELICOPTER) {
                // Helicopter shoots 3 bullets in a spread
                addBullet(px, py, BULLET_SPEED, 0, true);
                addBullet(px, py - 5, BULLET_SPEED, -0.5f, true);
                addBullet(px, py + 5, BULLET_SPEED, 0.5f, true);
            } else {
                // Jeep shoots 2 bullets
                addBullet(px, py - 5, BULLET_SPEED, 0, true);
                addBullet(px, py + 5, BULLET_SPEED, 0, true);
            }
        }
    }
    
    void addBullet(float x, float y, float vx, float vy, bool isPlayerBullet) {
        // Limit the number of bullets to prevent overflow
        if (bullets.size() < MAX_BULLETS) {
            bullets.push_back(std::make_unique<Bullet>(x, y, vx, vy, isPlayerBullet));
        }
    }
    
    void addExplosion(float x, float y) {
        explosions.push_back(std::make_unique<Explosion>(x, y));
    }
    
    bool isGameOver() const { return gameOver; }
    
    int getPlayerScore() const {
        return player ? player->getScore() : 0;
    }
    
    int getPlayerLives() const {
        return player ? player->getLives() : 0;
    }

private:
    void spawnEnemies() {
        std::uniform_real_distribution<float> spawnDist(0.0f, 1.0f);
        std::uniform_real_distribution<float> heightDist(50.0f, SCREEN_HEIGHT - 150.0f);
        std::uniform_int_distribution<int> typeDist(0, 2);
        std::uniform_real_distribution<float> speedDist(1.0f, 3.0f);
        
        if (spawnDist(rng) < ENEMY_SPAWN_RATE) {
            float speed = speedDist(rng);
            int type = typeDist(rng);
            float y;
            EntityType enemyType;
            
            switch (type) {
                case 0: // Air enemy
                    y = heightDist(rng);
                    enemyType = EntityType::ENEMY_AIR;
                    break;
                case 1: // Ground enemy
                    y = SCREEN_HEIGHT - ENEMY_HEIGHT - 50;
                    enemyType = EntityType::ENEMY_GROUND;
                    break;
                case 2: // Turret
                    y = SCREEN_HEIGHT - ENEMY_HEIGHT - 50;
                    enemyType = EntityType::ENEMY_TURRET;
                    break;
                default:
                    y = heightDist(rng);
                    enemyType = EntityType::ENEMY_AIR;
            }
            
            enemies.push_back(std::make_unique<Enemy>(SCREEN_WIDTH, y, enemyType, speed));
        }
    }
    
    void checkCollisions() {
        if (!player) return;
        
        // Check player bullets against enemies
        for (auto& bullet : bullets) {
            if (bullet->getType() == EntityType::BULLET_PLAYER) {
                for (auto& enemy : enemies) {
                    if (bullet->isAlive() && enemy->isAlive() && bullet->checkCollision(*enemy)) {
                        bullet->destroy();
                        enemy->destroy();
                        addExplosion(enemy->getX() + enemy->getWidth()/2, enemy->getY() + enemy->getHeight()/2);
                        player->increaseScore(100);
                        break;
                    }
                }
            }
        }
        
        // Check enemy bullets against player
        for (auto& bullet : bullets) {
            if (bullet->getType() == EntityType::BULLET_ENEMY) {
                if (bullet->isAlive() && player->isAlive() && bullet->checkCollision(*player)) {
                    bullet->destroy();
                    player->loseLife();
                    addExplosion(player->getX() + player->getWidth()/2, player->getY() + player->getHeight()/2);
                    break;
                }
            }
        }
        
        // Check player against enemies (collision damage)
        for (auto& enemy : enemies) {
            if (enemy->isAlive() && player->isAlive() && player->checkCollision(*enemy)) {
                enemy->destroy();
                player->loseLife();
                addExplosion(enemy->getX() + enemy->getWidth()/2, enemy->getY() + enemy->getHeight()/2);
            }
        }
    }
    
    void cleanup() {
        // Remove destroyed objects
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(), 
                [](const auto& b) { return !b->isAlive(); }),
            bullets.end());
            
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(), 
                [](const auto& e) { return !e->isAlive(); }),
            enemies.end());
            
        explosions.erase(
            std::remove_if(explosions.begin(), explosions.end(), 
                [](const auto& e) { return !e->isAlive(); }),
            explosions.end());
    }
    
    void renderHUD(SDL_Renderer* renderer) {
        // This is a very basic HUD implementation
        // In a real game, you'd use SDL_ttf to render text
        
        // Score bar
        SDL_Rect scoreBar = {10, 10, 150, 30};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200);
        SDL_RenderFillRect(renderer, &scoreBar);
        
        // Lives
        for (int i = 0; i < getPlayerLives(); i++) {
            SDL_Rect lifeIcon = {200 + i * 30, 10, 20, 20};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &lifeIcon);
        }
        
        // Game over message
        if (gameOver) {
            SDL_Rect gameOverBox = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 30, 200, 60};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_RenderFillRect(renderer, &gameOverBox);
        }
    }
    
    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::unique_ptr<Bullet>> bullets;
    std::vector<std::unique_ptr<Explosion>> explosions;
    std::vector<std::unique_ptr<BackgroundLayer>> backgrounds;
    bool gameOver;
    int levelDistance;
    std::mt19937 rng;
};

// Main application class
class SilkwormGame {
public:
    SilkwormGame() : window(nullptr), renderer(nullptr), running(false) {}
    
    ~SilkwormGame() {
        cleanup();
    }
    
    bool init() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Initialize SDL_image
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
            return false;
        }
        
        // Create window
        window = SDL_CreateWindow("Silkworm Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
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
        
        return true;
    }
    
    void run() {
        if (!init()) {
            return;
        }
        
        // Game state variables
        enum class GameState {
            MENU,
            PLAYING,
            GAME_OVER
        };
        
        GameState gameState = GameState::MENU;
        PlayerType selectedPlayer = PlayerType::HELICOPTER;
        GameWorld gameWorld;
        
        running = true;
        Uint32 lastTime = SDL_GetTicks();
        constexpr int FPS = 60;
        constexpr int frameDelay = 1000 / FPS;
        
        while (running) {
            Uint32 startTime = SDL_GetTicks();
            
            // Handle events
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    running = false;
                } else if (e.type == SDL_KEYDOWN) {
                    switch (gameState) {
                        case GameState::MENU:
                            if (e.key.keysym.sym == SDLK_h) {
                                selectedPlayer = PlayerType::HELICOPTER;
                                gameWorld = GameWorld();
                                gameWorld.init(selectedPlayer);
                                gameState = GameState::PLAYING;
                            } else if (e.key.keysym.sym == SDLK_j) {
                                selectedPlayer = PlayerType::JEEP;
                                gameWorld = GameWorld();
                                gameWorld.init(selectedPlayer);
                                gameState = GameState::PLAYING;
                            }
                            break;
                            
                        case GameState::PLAYING:
                            if (e.key.keysym.sym == SDLK_ESCAPE) {
                                gameState = GameState::MENU;
                            }
                            break;
                            
                        case GameState::GAME_OVER:
                            if (e.key.keysym.sym == SDLK_RETURN) {
                                gameState = GameState::MENU;
                            }
                            break;
                    }
                }
            }
            
            // Get keyboard state
            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            
            // Update game logic
            if (gameState == GameState::PLAYING) {
                gameWorld.handleInput(keystate);
                gameWorld.update();
                
                if (gameWorld.isGameOver()) {
                    gameState = GameState::GAME_OVER;
                }
            }
            
            // Clear screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            // Render game
            switch (gameState) {
                case GameState::MENU:
                    renderMenu();
                    break;
                    
                case GameState::PLAYING:
                    gameWorld.render(renderer);
                    break;
                    
                case GameState::GAME_OVER:
                    gameWorld.render(renderer);
                    renderGameOver(gameWorld.getPlayerScore());
                    break;
            }
            
            // Update screen
            SDL_RenderPresent(renderer);
            
            // Cap to 60 FPS
            Uint32 frameTime = SDL_GetTicks() - startTime;
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
    }

private:
    void renderMenu() {
        // Render title
        SDL_Rect titleRect = {SCREEN_WIDTH/2 - 150, 100, 300, 60};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &titleRect);
        
        // Render option boxes
        SDL_Rect helicopterBox = {SCREEN_WIDTH/2 - 150, 200, 300, 50};
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &helicopterBox);
        
        SDL_Rect jeepBox = {SCREEN_WIDTH/2 - 150, 270, 300, 50};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &jeepBox);
        
        // In a real game, you would use SDL_ttf for text rendering
    }
    
    void renderGameOver(int score) {
        SDL_Rect gameOverBox = {SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 75, 300, 150};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_RenderFillRect(renderer, &gameOverBox);
        
        // In a real game, you would render text showing the score
    }
    
    void cleanup() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        
        IMG_Quit();
        SDL_Quit();
    }
    
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
};

int main(int argc, char* argv[]) {
    SilkwormGame game;
    game.run();
    return 0;
}
