#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

// Game constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_SPEED = 5;
const int BULLET_SPEED = 10;
const int ENEMY_SPEED = 2;
const float PI = 3.14159f;

// Game structures
struct GameObject {
    float x, y;
    float angle;
    int width, height;
    SDL_Texture* texture;
    bool active;
};

struct Player : GameObject {
    int health;
    int score;
};

struct Bullet : GameObject {
    float dx, dy;
};

struct Enemy : GameObject {
    int health;
};

// Function prototypes
bool init();
void close();
SDL_Texture* loadTexture(const char* path);
void handleEvents(SDL_Event& e, Player& player, bool& quit, std::vector<Bullet>& bullets);
void updatePlayer(Player& player);
void updateBullets(std::vector<Bullet>& bullets);
void updateEnemies(std::vector<Enemy>& enemies, Player& player);
void spawnEnemy(std::vector<Enemy>& enemies);
void checkCollisions(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies);
void render(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies);

// Global variables
SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gPlayerTexture = nullptr;
SDL_Texture* gBulletTexture = nullptr;
SDL_Texture* gEnemyTexture = nullptr;
SDL_Texture* gBackgroundTexture = nullptr;

std::random_device rd;
std::mt19937 gen(rd());

int main(int argc, char* args[]) {
    if (!init()) {
        std::cout << "Failed to initialize!" << std::endl;
        return -1;
    }

    // Create game objects
    Player player = {
        SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, // x, y
        0.0f, // angle
        32, 32, // width, height
        gPlayerTexture,
        true, // active
        100, // health
        0 // score
    };

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;

    // Game loop variables
    bool quit = false;
    SDL_Event e;
    Uint32 lastEnemySpawn = 0;
    Uint32 enemySpawnInterval = 2000; // 2 seconds

    // Game loop
    while (!quit) {
        // Handle events
        handleEvents(e, player, quit, bullets);

        // Spawn enemies periodically
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastEnemySpawn > enemySpawnInterval) {
            spawnEnemy(enemies);
            lastEnemySpawn = currentTime;
            // Increase difficulty over time
            if (enemySpawnInterval > 500) {
                enemySpawnInterval -= 50;
            }
        }

        // Update game objects
        updatePlayer(player);
        updateBullets(bullets);
        updateEnemies(enemies, player);
        checkCollisions(player, bullets, enemies);

        // Render
        render(player, bullets, enemies);

        // Slight delay to control frame rate
        SDL_Delay(16); // ~60 FPS

        // Check for game over
        if (player.health <= 0 || !player.active) {
            std::cout << "Game Over! Final Score: " << player.score << std::endl;
            SDL_Delay(3000);
            quit = true;
        }
    }

    close();
    return 0;
}

bool init() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Create window
    gWindow = SDL_CreateWindow("Seek and Destroy Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == nullptr) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == nullptr) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // In a real implementation, we would load actual image files
    // For this example, we'll create simple colored rectangles
    
    // Create player texture (blue rectangle)
    SDL_Surface* playerSurface = SDL_CreateRGBSurface(0, 32, 32, 32, 0, 0, 0, 0);
    SDL_FillRect(playerSurface, NULL, SDL_MapRGB(playerSurface->format, 0, 0, 255));
    gPlayerTexture = SDL_CreateTextureFromSurface(gRenderer, playerSurface);
    SDL_FreeSurface(playerSurface);
    
    // Create bullet texture (red rectangle)
    SDL_Surface* bulletSurface = SDL_CreateRGBSurface(0, 8, 8, 32, 0, 0, 0, 0);
    SDL_FillRect(bulletSurface, NULL, SDL_MapRGB(bulletSurface->format, 255, 0, 0));
    gBulletTexture = SDL_CreateTextureFromSurface(gRenderer, bulletSurface);
    SDL_FreeSurface(bulletSurface);
    
    // Create enemy texture (green rectangle)
    SDL_Surface* enemySurface = SDL_CreateRGBSurface(0, 32, 32, 32, 0, 0, 0, 0);
    SDL_FillRect(enemySurface, NULL, SDL_MapRGB(enemySurface->format, 0, 255, 0));
    gEnemyTexture = SDL_CreateTextureFromSurface(gRenderer, enemySurface);
    SDL_FreeSurface(enemySurface);
    
    // Create background texture (dark gray)
    SDL_Surface* bgSurface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    SDL_FillRect(bgSurface, NULL, SDL_MapRGB(bgSurface->format, 30, 30, 30));
    gBackgroundTexture = SDL_CreateTextureFromSurface(gRenderer, bgSurface);
    SDL_FreeSurface(bgSurface);

    return true;
}

void close() {
    // Free textures
    SDL_DestroyTexture(gPlayerTexture);
    SDL_DestroyTexture(gBulletTexture);
    SDL_DestroyTexture(gEnemyTexture);
    SDL_DestroyTexture(gBackgroundTexture);
    
    // Destroy window and renderer
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    
    // Quit SDL subsystems
    IMG_Quit();
    SDL_Quit();
}

void handleEvents(SDL_Event& e, Player& player, bool& quit, std::vector<Bullet>& bullets) {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
            else if (e.key.keysym.sym == SDLK_SPACE) {
                // Shoot a bullet in the direction the player is facing
                Bullet bullet = {
                    player.x + player.width / 2.0f, player.y + player.height / 2.0f, // x, y
                    player.angle, // angle
                    8, 8, // width, height
                    gBulletTexture,
                    true, // active
                    cos(player.angle * PI / 180.0f) * BULLET_SPEED, // dx
                    sin(player.angle * PI / 180.0f) * BULLET_SPEED  // dy
                };
                bullets.push_back(bullet);
            }
        }
        else if (e.type == SDL_MOUSEMOTION) {
            // Calculate angle between player and mouse
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            float dx = mouseX - (player.x + player.width / 2.0f);
            float dy = mouseY - (player.y + player.height / 2.0f);
            player.angle = atan2(dy, dx) * 180.0f / PI;
        }
    }

    // Handle continuous key presses
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    if (keystates[SDL_SCANCODE_W]) player.y -= PLAYER_SPEED;
    if (keystates[SDL_SCANCODE_S]) player.y += PLAYER_SPEED;
    if (keystates[SDL_SCANCODE_A]) player.x -= PLAYER_SPEED;
    if (keystates[SDL_SCANCODE_D]) player.x += PLAYER_SPEED;
}

void updatePlayer(Player& player) {
    // Keep player in bounds
    if (player.x < 0) player.x = 0;
    if (player.y < 0) player.y = 0;
    if (player.x > SCREEN_WIDTH - player.width) player.x = SCREEN_WIDTH - player.width;
    if (player.y > SCREEN_HEIGHT - player.height) player.y = SCREEN_HEIGHT - player.height;
}

void updateBullets(std::vector<Bullet>& bullets) {
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.x += bullet.dx;
            bullet.y += bullet.dy;
            
            // Remove bullets that go out of bounds
            if (bullet.x < 0 || bullet.x > SCREEN_WIDTH || bullet.y < 0 || bullet.y > SCREEN_HEIGHT) {
                bullet.active = false;
            }
        }
    }
    
    // Remove inactive bullets
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), 
        [](const Bullet& b) { return !b.active; }), bullets.end());
}

void updateEnemies(std::vector<Enemy>& enemies, Player& player) {
    for (auto& enemy : enemies) {
        if (enemy.active) {
            // Move towards player
            float dx = player.x - enemy.x;
            float dy = player.y - enemy.y;
            float length = sqrt(dx * dx + dy * dy);
            
            if (length > 0) {
                dx /= length;
                dy /= length;
                enemy.x += dx * ENEMY_SPEED;
                enemy.y += dy * ENEMY_SPEED;
                enemy.angle = atan2(dy, dx) * 180.0f / PI;
            }
        }
    }
    
    // Remove inactive enemies
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), 
        [](const Enemy& e) { return !e.active; }), enemies.end());
}

void spawnEnemy(std::vector<Enemy>& enemies) {
    std::uniform_int_distribution<> distr_side(0, 3);
    std::uniform_int_distribution<> distr_x(0, SCREEN_WIDTH);
    std::uniform_int_distribution<> distr_y(0, SCREEN_HEIGHT);
    
    int side = distr_side(gen);
    float x, y;
    
    // Spawn from one of the four sides
    switch (side) {
        case 0: // Top
            x = distr_x(gen);
            y = -32;
            break;
        case 1: // Right
            x = SCREEN_WIDTH + 32;
            y = distr_y(gen);
            break;
        case 2: // Bottom
            x = distr_x(gen);
            y = SCREEN_HEIGHT + 32;
            break;
        case 3: // Left
            x = -32;
            y = distr_y(gen);
            break;
    }
    
    Enemy enemy = {
        x, y, // x, y
        0.0f, // angle
        32, 32, // width, height
        gEnemyTexture,
        true, // active
        1 // health
    };
    
    enemies.push_back(enemy);
}

void checkCollisions(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies) {
    // Check bullet-enemy collisions
    for (auto& bullet : bullets) {
        if (!bullet.active) continue;
        
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            
            // Simple AABB collision detection
            bool collision = (bullet.x < enemy.x + enemy.width &&
                             bullet.x + bullet.width > enemy.x &&
                             bullet.y < enemy.y + enemy.height &&
                             bullet.y + bullet.height > enemy.y);
                             
            if (collision) {
                bullet.active = false;
                enemy.health--;
                
                if (enemy.health <= 0) {
                    enemy.active = false;
                    player.score += 10;
                }
                
                break;
            }
        }
    }
    
    // Check player-enemy collisions
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        
        // Simple AABB collision detection
        bool collision = (player.x < enemy.x + enemy.width &&
                         player.x + player.width > enemy.x &&
                         player.y < enemy.y + enemy.height &&
                         player.y + player.height > enemy.y);
                         
        if (collision) {
            player.health -= 10;
            enemy.active = false;
            break;
        }
    }
}

void render(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies) {
    // Clear screen
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

    // Render background
    SDL_RenderCopy(gRenderer, gBackgroundTexture, NULL, NULL);

    // Render bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            SDL_Rect destRect = { 
                static_cast<int>(bullet.x), 
                static_cast<int>(bullet.y), 
                bullet.width, 
                bullet.height 
            };
            SDL_RenderCopy(gRenderer, bullet.texture, NULL, &destRect);
        }
    }
    
    // Render enemies
    for (const auto& enemy : enemies) {
        if (enemy.active) {
            SDL_Rect destRect = { 
                static_cast<int>(enemy.x), 
                static_cast<int>(enemy.y), 
                enemy.width, 
                enemy.height 
            };
            SDL_RenderCopyEx(gRenderer, enemy.texture, NULL, &destRect, enemy.angle, NULL, SDL_FLIP_NONE);
        }
    }
    
    // Render player
    SDL_Rect playerRect = { 
        static_cast<int>(player.x), 
        static_cast<int>(player.y), 
        player.width, 
        player.height 
    };
    SDL_RenderCopyEx(gRenderer, player.texture, NULL, &playerRect, player.angle, NULL, SDL_FLIP_NONE);
    
    // Render score and health text (would use SDL_ttf in a real implementation)
    
    // Update screen
    SDL_RenderPresent(gRenderer);
}
