#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

// Constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_SIZE = 32;
const int BASE_SIZE = 64;
const int MINE_SIZE = 16;
const int BULLET_SIZE = 8;
const int ENEMY_SIZE = 24;
const float PLAYER_SPEED = 5.0f;
const float BULLET_SPEED = 10.0f;
const float ENEMY_SPEED = 2.0f;
const int MAX_BASES = 6;
const int MAX_MINES = 20;
const int MAX_ENEMIES = 5;
const int WORLD_SIZE = 2000; // Size of the game world

// Game objects
struct GameObject {
    float x, y;
    float dx, dy;
    bool active;
    SDL_Rect rect;
    
    GameObject() : x(0), y(0), dx(0), dy(0), active(true) {
        rect = {0, 0, 0, 0};
    }
    
    virtual void update() {
        x += dx;
        y += dy;
        
        // Update the rectangle for rendering and collision detection
        rect.x = static_cast<int>(x);
        rect.y = static_cast<int>(y);
    }
    
    bool collidesWith(const GameObject& other) const {
        return SDL_HasIntersection(&rect, &other.rect);
    }
};

struct Player : GameObject {
    int lives;
    int score;
    float angle;
    Uint32 lastShotTime;
    bool invulnerable;
    Uint32 invulnerableUntil;
    
    Player() : lives(3), score(0), angle(0), lastShotTime(0), invulnerable(false), invulnerableUntil(0) {
        rect.w = PLAYER_SIZE;
        rect.h = PLAYER_SIZE;
    }
    
    void update() override {
        GameObject::update();
        
        // Keep player within world bounds
        x = std::max(0.0f, std::min(x, static_cast<float>(WORLD_SIZE - rect.w)));
        y = std::max(0.0f, std::min(y, static_cast<float>(WORLD_SIZE - rect.h)));
        
        // Update invulnerability
        if (invulnerable && SDL_GetTicks() > invulnerableUntil) {
            invulnerable = false;
        }
    }
    
    void makeInvulnerable(Uint32 duration) {
        invulnerable = true;
        invulnerableUntil = SDL_GetTicks() + duration;
    }
};

struct Bullet : GameObject {
    bool isPlayerBullet;
    
    Bullet() : isPlayerBullet(true) {
        rect.w = BULLET_SIZE;
        rect.h = BULLET_SIZE;
        active = false;
    }
    
    void fire(float startX, float startY, float angle, bool fromPlayer) {
        x = startX;
        y = startY;
        dx = BULLET_SPEED * std::cos(angle);
        dy = BULLET_SPEED * std::sin(angle);
        isPlayerBullet = fromPlayer;
        active = true;
    }
    
    void update() override {
        GameObject::update();
        
        // Deactivate bullets that go out of bounds
        if (x < 0 || x > WORLD_SIZE || y < 0 || y > WORLD_SIZE) {
            active = false;
        }
    }
};

struct Base : GameObject {
    int health;
    bool destroyed;
    
    Base() : health(5), destroyed(false) {
        rect.w = BASE_SIZE;
        rect.h = BASE_SIZE;
        active = true;
    }
    
    void hit() {
        health--;
        if (health <= 0) {
            destroyed = true;
            active = false;
        }
    }
};

struct Mine : GameObject {
    Mine() {
        rect.w = MINE_SIZE;
        rect.h = MINE_SIZE;
        active = true;
    }
};

struct Enemy : GameObject {
    float angle;
    Uint32 lastShotTime;
    Uint32 changeDirectionTime;
    
    Enemy() : angle(0), lastShotTime(0), changeDirectionTime(0) {
        rect.w = ENEMY_SIZE;
        rect.h = ENEMY_SIZE;
        active = false;
    }
    
    void activate(float startX, float startY) {
        x = startX;
        y = startY;
        angle = static_cast<float>(rand() % 360) * M_PI / 180.0f;
        dx = ENEMY_SPEED * std::cos(angle);
        dy = ENEMY_SPEED * std::sin(angle);
        lastShotTime = SDL_GetTicks();
        changeDirectionTime = SDL_GetTicks() + rand() % 3000 + 1000;
        active = true;
    }
    
    void update() override {
        GameObject::update();
        
        // Change direction periodically
        if (SDL_GetTicks() > changeDirectionTime) {
            angle = static_cast<float>(rand() % 360) * M_PI / 180.0f;
            dx = ENEMY_SPEED * std::cos(angle);
            dy = ENEMY_SPEED * std::sin(angle);
            changeDirectionTime = SDL_GetTicks() + rand() % 3000 + 1000;
        }
        
        // Keep enemy within world bounds
        if (x < 0 || x > WORLD_SIZE - rect.w) {
            dx = -dx;
            angle = atan2(dy, dx);
        }
        if (y < 0 || y > WORLD_SIZE - rect.h) {
            dy = -dy;
            angle = atan2(dy, dx);
        }
    }
};

// Camera to follow player
struct Camera {
    int x, y;
    int width, height;
    
    Camera() : x(0), y(0), width(SCREEN_WIDTH), height(SCREEN_HEIGHT) {}
    
    void update(const Player& player) {
        // Center the camera on the player
        x = static_cast<int>(player.x + player.rect.w / 2 - width / 2);
        y = static_cast<int>(player.y + player.rect.h / 2 - height / 2);
        
        // Keep the camera within the world bounds
        x = std::max(0, std::min(x, WORLD_SIZE - width));
        y = std::max(0, std::min(y, WORLD_SIZE - height));
    }
    
    SDL_Rect getWorldToScreenRect(const SDL_Rect& worldRect) const {
        SDL_Rect screenRect = {
            worldRect.x - x,
            worldRect.y - y,
            worldRect.w,
            worldRect.h
        };
        return screenRect;
    }
    
    bool isVisible(const SDL_Rect& worldRect) const {
        return (worldRect.x + worldRect.w > x && worldRect.x < x + width &&
                worldRect.y + worldRect.h > y && worldRect.y < y + height);
    }
};

// Game class
class BosconianGame {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    
    // Textures
    SDL_Texture* playerTexture;
    SDL_Texture* bulletTexture;
    SDL_Texture* baseTexture;
    SDL_Texture* mineTexture;
    SDL_Texture* enemyTexture;
    SDL_Texture* backgroundTexture;
    
    // Game objects
    Player player;
    std::vector<Bullet> bullets;
    std::vector<Base> bases;
    std::vector<Mine> mines;
    std::vector<Enemy> enemies;
    
    // Camera
    Camera camera;
    
    // Random number generator
    std::mt19937 rng;
    
    // Game state
    bool gameOver;
    Uint32 lastEnemySpawnTime;
    
public:
    BosconianGame() : window(nullptr), renderer(nullptr), running(false), gameOver(false), lastEnemySpawnTime(0) {
        // Initialize random number generator
        std::random_device rd;
        rng = std::mt19937(rd());
    }
    
    ~BosconianGame() {
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
        window = SDL_CreateWindow("Bosconian Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == nullptr) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer == nullptr) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create textures
        SDL_Surface* surface = SDL_CreateRGBSurface(0, PLAYER_SIZE, PLAYER_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 0, 255, 0));
        playerTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        surface = SDL_CreateRGBSurface(0, BULLET_SIZE, BULLET_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 255, 0));
        bulletTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        surface = SDL_CreateRGBSurface(0, BASE_SIZE, BASE_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 0, 0));
        baseTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        surface = SDL_CreateRGBSurface(0, MINE_SIZE, MINE_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 165, 0));
        mineTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        surface = SDL_CreateRGBSurface(0, ENEMY_SIZE, ENEMY_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 0, 255));
        enemyTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        // Create a starfield background
        surface = SDL_CreateRGBSurface(0, WORLD_SIZE, WORLD_SIZE, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 0, 0, 0));
        
        // Add stars
        SDL_Rect starRect = {0, 0, 2, 2};
        for (int i = 0; i < 1000; ++i) {
            starRect.x = rand() % WORLD_SIZE;
            starRect.y = rand() % WORLD_SIZE;
            SDL_FillRect(surface, &starRect, SDL_MapRGB(surface->format, 255, 255, 255));
        }
        
        backgroundTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        // Initialize bullets pool
        bullets.resize(100);
        
        // Initialize game objects
        resetGame();
        
        running = true;
        return true;
    }
    
    void resetGame() {
        // Reset player
        player.x = WORLD_SIZE / 2 - PLAYER_SIZE / 2;
        player.y = WORLD_SIZE / 2 - PLAYER_SIZE / 2;
        player.dx = 0;
        player.dy = 0;
        player.angle = 0;
        player.lives = 3;
        player.score = 0;
        player.invulnerable = false;
        
        // Reset bullets
        for (auto& bullet : bullets) {
            bullet.active = false;
        }
        
        // Create bases
        bases.clear();
        std::uniform_int_distribution<int> dist(100, WORLD_SIZE - 100 - BASE_SIZE);
        
        for (int i = 0; i < MAX_BASES; ++i) {
            Base base;
            bool validPosition;
            
            do {
                validPosition = true;
                base.x = dist(rng);
                base.y = dist(rng);
                base.rect.x = static_cast<int>(base.x);
                base.rect.y = static_cast<int>(base.y);
                
                // Check distance from player
                float dx = base.x - player.x;
                float dy = base.y - player.y;
                float distSq = dx * dx + dy * dy;
                
                if (distSq < 200 * 200) {
                    validPosition = false;
                    continue;
                }
                
                // Check distance from other bases
                for (const auto& otherBase : bases) {
                    dx = base.x - otherBase.x;
                    dy = base.y - otherBase.y;
                    distSq = dx * dx + dy * dy;
                    
                    if (distSq < 200 * 200) {
                        validPosition = false;
                        break;
                    }
                }
            } while (!validPosition);
            
            bases.push_back(base);
        }
        
        // Create mines
        mines.clear();
        for (int i = 0; i < MAX_MINES; ++i) {
            Mine mine;
            bool validPosition;
            
            do {
                validPosition = true;
                mine.x = dist(rng);
                mine.y = dist(rng);
                mine.rect.x = static_cast<int>(mine.x);
                mine.rect.y = static_cast<int>(mine.y);
                
                // Check distance from player
                float dx = mine.x - player.x;
                float dy = mine.y - player.y;
                float distSq = dx * dx + dy * dy;
                
                if (distSq < 150 * 150) {
                    validPosition = false;
                    continue;
                }
                
                // Check distance from bases
                for (const auto& base : bases) {
                    dx = mine.x - base.x;
                    dy = mine.y - base.y;
                    distSq = dx * dx + dy * dy;
                    
                    if (distSq < 100 * 100) {
                        validPosition = false;
                        break;
                    }
                }
                
                // Check distance from other mines
                for (const auto& otherMine : mines) {
                    dx = mine.x - otherMine.x;
                    dy = mine.y - otherMine.y;
                    distSq = dx * dx + dy * dy;
                    
                    if (distSq < 50 * 50) {
                        validPosition = false;
                        break;
                    }
                }
            } while (!validPosition);
            
            mines.push_back(mine);
        }
        
        // Reset enemies
        enemies.clear();
        enemies.resize(MAX_ENEMIES);
        lastEnemySpawnTime = SDL_GetTicks();
        
        gameOver = false;
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
                    case SDLK_r:
                        if (gameOver) {
                            resetGame();
                        }
                        break;
                    case SDLK_SPACE:
                        if (!gameOver) {
                            fireBullet();
                        }
                        break;
                }
            }
        }
        
        if (!gameOver) {
            // Handle player movement
            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            player.dx = 0;
            player.dy = 0;
            
            if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W]) {
                player.dy = -PLAYER_SPEED;
                player.angle = -M_PI / 2;
            }
            if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S]) {
                player.dy = PLAYER_SPEED;
                player.angle = M_PI / 2;
            }
            if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) {
                player.dx = -PLAYER_SPEED;
                player.angle = M_PI;
                if (player.dy != 0) {
                    player.angle = player.dy < 0 ? -3 * M_PI / 4 : 3 * M_PI / 4;
                }
            }
            if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) {
                player.dx = PLAYER_SPEED;
                player.angle = 0;
                if (player.dy != 0) {
                    player.angle = player.dy < 0 ? -M_PI / 4 : M_PI / 4;
                }
            }
            
            // Automatic firing
            if (keystate[SDL_SCANCODE_SPACE]) {
                fireBullet();
            }
        }
    }
    
    void fireBullet() {
        // Limit fire rate
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - player.lastShotTime < 200) {
            return;
        }
        
        // Find an inactive bullet to reuse
        for (auto& bullet : bullets) {
            if (!bullet.active) {
                bullet.fire(
                    player.x + player.rect.w / 2 - BULLET_SIZE / 2,
                    player.y + player.rect.h / 2 - BULLET_SIZE / 2,
                    player.angle,
                    true
                );
                player.lastShotTime = currentTime;
                break;
            }
        }
    }
    
    void update() {
        if (gameOver) {
            return;
        }
        
        // Update player
        player.update();
        
        // Update camera
        camera.update(player);
        
        // Update bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.update();
            }
        }
        
        // Update enemies
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.update();
                
                // Enemy shooting
                Uint32 currentTime = SDL_GetTicks();
                if (currentTime - enemy.lastShotTime > 2000) {
                    // Find an inactive bullet to reuse
                    for (auto& bullet : bullets) {
                        if (!bullet.active) {
                            // Calculate angle to player
                            float dx = player.x - enemy.x;
                            float dy = player.y - enemy.y;
                            float angle = atan2(dy, dx);
                            
                            bullet.fire(
                                enemy.x + enemy.rect.w / 2 - BULLET_SIZE / 2,
                                enemy.y + enemy.rect.h / 2 - BULLET_SIZE / 2,
                                angle,
                                false
                            );
                            enemy.lastShotTime = currentTime;
                            break;
                        }
                    }
                }
            }
        }
        
        // Spawn enemies
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastEnemySpawnTime > 3000) {
            spawnEnemy();
            lastEnemySpawnTime = currentTime;
        }
        
        // Check for all bases destroyed
        bool allBasesDestroyed = true;
        for (const auto& base : bases) {
            if (base.active) {
                allBasesDestroyed = false;
                break;
            }
        }
        
        if (allBasesDestroyed) {
            // Player wins this level
            resetGame();
            player.score += 1000;
        }
        
        // Check collisions
        checkCollisions();
    }
    
    void spawnEnemy() {
        for (auto& enemy : enemies) {
            if (!enemy.active) {
                std::uniform_int_distribution<int> dist(0, WORLD_SIZE - ENEMY_SIZE);
                
                // Spawn away from player
                float x, y;
                float distSq;
                
                do {
                    x = dist(rng);
                    y = dist(rng);
                    float dx = x - player.x;
                    float dy = y - player.y;
                    distSq = dx * dx + dy * dy;
                } while (distSq < 300 * 300);
                
                enemy.activate(x, y);
                break;
            }
        }
    }
    
    void checkCollisions() {
        // Player collision with bases
        for (auto& base : bases) {
            if (base.active && !player.invulnerable && player.collidesWith(base)) {
                playerHit();
                break;
            }
        }
        
        // Player collision with mines
        for (auto& mine : mines) {
            if (mine.active && !player.invulnerable && player.collidesWith(mine)) {
                playerHit();
                mine.active = false;
                break;
            }
        }
        
        // Player collision with enemies
        for (auto& enemy : enemies) {
            if (enemy.active && !player.invulnerable && player.collidesWith(enemy)) {
                playerHit();
                enemy.active = false;
                break;
            }
        }
        
        // Bullet collisions
        for (auto& bullet : bullets) {
            if (bullet.active) {
                // Player bullets hitting bases
                if (bullet.isPlayerBullet) {
                    for (auto& base : bases) {
                        if (base.active && bullet.collidesWith(base)) {
                            base.hit();
                            bullet.active = false;
                            if (!base.active) {
                                player.score += 100;
                            }
                            break;
                        }
                    }
                    
                    // Player bullets hitting enemies
                    for (auto& enemy : enemies) {
                        if (enemy.active && bullet.collidesWith(enemy)) {
                            enemy.active = false;
                            bullet.active = false;
                            player.score += 50;
                            break;
                        }
                    }
                }
                // Enemy bullets hitting player
                else if (!player.invulnerable && bullet.collidesWith(player)) {
                    playerHit();
                    bullet.active = false;
                }
            }
        }
    }
    
    void playerHit() {
        player.lives--;
        if (player.lives <= 0) {
            gameOver = true;
        } else {
            player.makeInvulnerable(3000);
        }
    }
    
    void render() {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Render background (visible portion of the world)
        SDL_Rect srcRect = {camera.x, camera.y, camera.width, camera.height};
        SDL_Rect destRect = {0, 0, camera.width, camera.height};
        SDL_RenderCopy(renderer, backgroundTexture, &srcRect, &destRect);
        
        // Render bases
        for (const auto& base : bases) {
            if (base.active && camera.isVisible(base.rect)) {
                SDL_Rect screenRect = camera.getWorldToScreenRect(base.rect);
                SDL_RenderCopy(renderer, baseTexture, nullptr, &screenRect);
            }
        }
        
        // Render mines
        for (const auto& mine : mines) {
            if (mine.active && camera.isVisible(mine.rect)) {
                SDL_Rect screenRect = camera.getWorldToScreenRect(mine.rect);
                SDL_RenderCopy(renderer, mineTexture, nullptr, &screenRect);
            }
        }
        
        // Render bullets
        for (const auto& bullet : bullets) {
            if (bullet.active && camera.isVisible(bullet.rect)) {
                SDL_Rect screenRect = camera.getWorldToScreenRect(bullet.rect);
                SDL_RenderCopy(renderer, bulletTexture, nullptr, &screenRect);
            }
        }
        
        // Render enemies
        for (const auto& enemy : enemies) {
            if (enemy.active && camera.isVisible(enemy.rect)) {
                SDL_Rect screenRect = camera.getWorldToScreenRect(enemy.rect);
                SDL_RenderCopy(renderer, enemyTexture, nullptr, &screenRect);
            }
        }
        
        // Render player
        if (!gameOver) {
            SDL_Rect screenRect = camera.getWorldToScreenRect(player.rect);
            
            // Flash player if invulnerable
            if (!player.invulnerable || (SDL_GetTicks() / 100) % 2 == 0) {
                // Render with rotation
                SDL_RenderCopyEx(
                    renderer,
                    playerTexture,
                    nullptr,
                    &screenRect,
                    player.angle * 180.0 / M_PI,
                    nullptr,
                    SDL_FLIP_NONE
                );
            }
        }
        
        // Render UI
        renderUI();
        
        // Present the rendered frame
        SDL_RenderPresent(renderer);
    }
    
    void renderUI() {
        // Render score and lives at the top of the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_Rect uiRect = {0, 0, SCREEN_WIDTH, 40};
        SDL_RenderFillRect(renderer, &uiRect);
        
        // Would normally use SDL_ttf for text, but for simplicity we'll render score as squares
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect scoreRect = {10, 10, 20, 20};
        SDL_RenderDrawRect(renderer, &scoreRect);
        
        // Draw player lives
        for (int i = 0; i < player.lives; ++i) {
            SDL_Rect lifeRect = {SCREEN_WIDTH - 30 - i * 30, 10, 20, 20};
            SDL_RenderFillRect(renderer, &lifeRect);
        }
        
        if (gameOver) {
            // Display game over message
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_Rect gameOverRect = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 50, 300, 100};
            SDL_RenderFillRect(renderer, &gameOverRect);
            
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &gameOverRect);
            
            // Would normally display text with SDL_ttf
            // For now, just draw a red X
            SDL_RenderDrawLine(renderer, gameOverRect.x + 100, gameOverRect.y + 30, 
                               gameOverRect.x + 200, gameOverRect.y + 70);
            SDL_RenderDrawLine(renderer, gameOverRect.x + 200, gameOverRect.y + 30, 
                               gameOverRect.x + 100, gameOverRect.y + 70);
        }
    }
    
    void cleanup() {
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(bulletTexture);
        SDL_DestroyTexture(baseTexture);
        SDL_DestroyTexture(mineTexture);
        SDL_DestroyTexture(enemyTexture);
        SDL_DestroyTexture(backgroundTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
    }
    
    bool isRunning() const {
        return running;
    }
};

int main(int argc, char* argv[]) {
    BosconianGame game;
    
    if (!game.init()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }
    
    // Game loop
    while (game.isRunning()) {
        game.handleEvents();
        game.update();
        game.render();
    }
    
    return 0;
}
