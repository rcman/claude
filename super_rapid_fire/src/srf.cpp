#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 800
#define FPS 60
#define PLAYER_SPEED 5
#define BULLET_SPEED 10
#define ENEMY_SPEED 3
#define MAX_PLAYER_BULLETS 30
#define MAX_ENEMIES 10
#define MAX_ENEMY_BULLETS 50
#define ENEMY_SPAWN_DELAY 60 // frames

typedef struct {
    float x, y;
    int width, height;
    bool active;
} Entity;

typedef struct {
    Entity entity;
    int health;
    int score;
    int cooldown;
    int invincibility;
} Player;

typedef struct {
    Entity entity;
    int health;
    int type;
    int cooldown;
    float dx, dy;
} Enemy;

typedef struct {
    Entity entity;
    float dx, dy;
    int damage;
} Bullet;

typedef struct {
    int y;
    float speed;
} Background;

// Function prototypes
bool init(SDL_Window** window, SDL_Renderer** renderer);
void cleanup(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* textures[]);
void handleInput(SDL_Event* event, Player* player, bool* quit);
void updatePlayer(Player* player);
void spawnEnemy(Enemy enemies[], int maxEnemies);
void updateEnemies(Enemy enemies[], int maxEnemies);
void shootPlayerBullet(Player* player, Bullet bullets[], int maxBullets);
void shootEnemyBullet(Enemy* enemy, Bullet bullets[], int maxBullets, Player* player);
void updateBullets(Bullet bullets[], int maxBullets);
void checkCollisions(Player* player, Enemy enemies[], int maxEnemies, Bullet playerBullets[], int maxPlayerBullets, Bullet enemyBullets[], int maxEnemyBullets);
void updateBackground(Background* background);
void renderGame(SDL_Renderer* renderer, SDL_Texture* textures[], Player* player, Enemy enemies[], int maxEnemies, Bullet playerBullets[], int maxPlayerBullets, Bullet enemyBullets[], int maxEnemyBullets, Background* background);

enum TextureTypes {
    TEXTURE_BACKGROUND,
    TEXTURE_PLAYER,
    TEXTURE_ENEMY1,
    TEXTURE_ENEMY2,
    TEXTURE_PLAYER_BULLET,
    TEXTURE_ENEMY_BULLET,
    TEXTURE_COUNT // Total number of textures
};

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* textures[TEXTURE_COUNT] = {NULL};
    
    if (!init(&window, &renderer)) {
        return 1;
    }
    
    // Load textures
    // In a real game, you'd use actual sprites
    SDL_Surface* tempSurface = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);
    
    // Background (blue)
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 0, 0, 50));
    textures[TEXTURE_BACKGROUND] = SDL_CreateTextureFromSurface(renderer, tempSurface);
    
    // Player (green)
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 0, 255, 0));
    textures[TEXTURE_PLAYER] = SDL_CreateTextureFromSurface(renderer, tempSurface);
    
    // Enemy1 (red)
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 255, 0, 0));
    textures[TEXTURE_ENEMY1] = SDL_CreateTextureFromSurface(renderer, tempSurface);
    
    // Enemy2 (purple)
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 255, 0, 255));
    textures[TEXTURE_ENEMY2] = SDL_CreateTextureFromSurface(renderer, tempSurface);
    
    // Player bullet (cyan)
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 0, 255, 255));
    textures[TEXTURE_PLAYER_BULLET] = SDL_CreateTextureFromSurface(renderer, tempSurface);
    
    // Enemy bullet (yellow)
    SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 255, 255, 0));
    textures[TEXTURE_ENEMY_BULLET] = SDL_CreateTextureFromSurface(renderer, tempSurface);
    
    SDL_FreeSurface(tempSurface);
    
    // Initialize game objects
    Player player = {
        .entity = {
            .x = WINDOW_WIDTH / 2 - 25,
            .y = WINDOW_HEIGHT - 100,
            .width = 50,
            .height = 50,
            .active = true
        },
        .health = 3,
        .score = 0,
        .cooldown = 0,
        .invincibility = 0
    };
    
    Enemy enemies[MAX_ENEMIES] = {0};
    Bullet playerBullets[MAX_PLAYER_BULLETS] = {0};
    Bullet enemyBullets[MAX_ENEMY_BULLETS] = {0};
    
    Background background = {
        .y = 0,
        .speed = 2.0f
    };
    
    srand(time(NULL));
    
    // Game loop variables
    bool quit = false;
    SDL_Event event;
    Uint32 frameStart;
    int frameTime;
    int enemySpawnTimer = 0;
    
    // Game loop
    while (!quit) {
        frameStart = SDL_GetTicks();
        
        // Process events
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            handleInput(&event, &player, &quit);
        }
        
        // Update game state
        updatePlayer(&player);
        
        // Enemy spawning
        enemySpawnTimer++;
        if (enemySpawnTimer >= ENEMY_SPAWN_DELAY) {
            spawnEnemy(enemies, MAX_ENEMIES);
            enemySpawnTimer = 0;
        }
        
        updateEnemies(enemies, MAX_ENEMIES);
        
        // Auto-shooting for player
        if (player.cooldown <= 0 && player.entity.active) {
            shootPlayerBullet(&player, playerBullets, MAX_PLAYER_BULLETS);
            player.cooldown = 10; // Shoot every 10 frames
        } else {
            player.cooldown--;
        }
        
        // Enemy shooting
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].entity.active) {
                if (enemies[i].cooldown <= 0) {
                    shootEnemyBullet(&enemies[i], enemyBullets, MAX_ENEMY_BULLETS, &player);
                    enemies[i].cooldown = 60 + rand() % 60; // Random cooldown between 60-120 frames
                } else {
                    enemies[i].cooldown--;
                }
            }
        }
        
        updateBullets(playerBullets, MAX_PLAYER_BULLETS);
        updateBullets(enemyBullets, MAX_ENEMY_BULLETS);
        updateBackground(&background);
        
        checkCollisions(&player, enemies, MAX_ENEMIES, playerBullets, MAX_PLAYER_BULLETS, enemyBullets, MAX_ENEMY_BULLETS);
        
        // Render
        renderGame(renderer, textures, &player, enemies, MAX_ENEMIES, playerBullets, MAX_PLAYER_BULLETS, enemyBullets, MAX_ENEMY_BULLETS, &background);
        
        // Cap the frame rate
        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 1000 / FPS) {
            SDL_Delay((1000 / FPS) - frameTime);
        }
    }
    
    cleanup(window, renderer, textures);
    return 0;
}

bool init(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    *window = SDL_CreateWindow("Super Rapid Fire Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

void cleanup(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* textures[]) {
    for (int i = 0; i < TEXTURE_COUNT; i++) {
        if (textures[i] != NULL) {
            SDL_DestroyTexture(textures[i]);
        }
    }
    
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
    }
    
    if (window != NULL) {
        SDL_DestroyWindow(window);
    }
    
    SDL_Quit();
}

void handleInput(SDL_Event* event, Player* player, bool* quit) {
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        switch (event->key.keysym.sym) {
            case SDLK_ESCAPE:
                *quit = true;
                break;
        }
    }
    
    // Continuous movement using keyboard state
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
    
    if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A]) {
        player->entity.x -= PLAYER_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D]) {
        player->entity.x += PLAYER_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W]) {
        player->entity.y -= PLAYER_SPEED;
    }
    if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S]) {
        player->entity.y += PLAYER_SPEED;
    }
}

void updatePlayer(Player* player) {
    // Keep player within screen bounds
    if (player->entity.x < 0) {
        player->entity.x = 0;
    } else if (player->entity.x > WINDOW_WIDTH - player->entity.width) {
        player->entity.x = WINDOW_WIDTH - player->entity.width;
    }
    
    if (player->entity.y < 0) {
        player->entity.y = 0;
    } else if (player->entity.y > WINDOW_HEIGHT - player->entity.height) {
        player->entity.y = WINDOW_HEIGHT - player->entity.height;
    }
    
    // Update invincibility frames
    if (player->invincibility > 0) {
        player->invincibility--;
    }
}

void spawnEnemy(Enemy enemies[], int maxEnemies) {
    for (int i = 0; i < maxEnemies; i++) {
        if (!enemies[i].entity.active) {
            enemies[i].entity.x = rand() % (WINDOW_WIDTH - 50);
            enemies[i].entity.y = -50;
            enemies[i].entity.width = 50;
            enemies[i].entity.height = 50;
            enemies[i].entity.active = true;
            enemies[i].health = 1 + rand() % 2; // 1 or 2 HP
            enemies[i].type = rand() % 2; // 0 = straight, 1 = wavy
            enemies[i].cooldown = 60 + rand() % 60;
            
            // Set movement pattern
            if (enemies[i].type == 0) {
                enemies[i].dx = 0;
                enemies[i].dy = ENEMY_SPEED;
            } else {
                enemies[i].dx = (rand() % 5 - 2) * 0.5f; // -1 to 1 in 0.5 increments
                enemies[i].dy = ENEMY_SPEED * 0.8f;
            }
            
            break;
        }
    }
}

void updateEnemies(Enemy enemies[], int maxEnemies) {
    for (int i = 0; i < maxEnemies; i++) {
        if (enemies[i].entity.active) {
            // Update position based on enemy type
            if (enemies[i].type == 0) {
                // Straight down
                enemies[i].entity.y += enemies[i].dy;
            } else {
                // Wavy pattern
                enemies[i].entity.x += enemies[i].dx;
                enemies[i].entity.y += enemies[i].dy;
                
                // Bounce off screen edges
                if (enemies[i].entity.x <= 0 || enemies[i].entity.x >= WINDOW_WIDTH - enemies[i].entity.width) {
                    enemies[i].dx = -enemies[i].dx;
                }
            }
            
            // Remove if off screen
            if (enemies[i].entity.y > WINDOW_HEIGHT) {
                enemies[i].entity.active = false;
            }
        }
    }
}

void shootPlayerBullet(Player* player, Bullet bullets[], int maxBullets) {
    for (int i = 0; i < maxBullets; i++) {
        if (!bullets[i].entity.active) {
            bullets[i].entity.x = player->entity.x + player->entity.width / 2 - 5;
            bullets[i].entity.y = player->entity.y;
            bullets[i].entity.width = 10;
            bullets[i].entity.height = 20;
            bullets[i].entity.active = true;
            bullets[i].dx = 0;
            bullets[i].dy = -BULLET_SPEED;
            bullets[i].damage = 1;
            break;
        }
    }
}

void shootEnemyBullet(Enemy* enemy, Bullet bullets[], int maxBullets, Player* player) {
    for (int i = 0; i < maxBullets; i++) {
        if (!bullets[i].entity.active) {
            bullets[i].entity.x = enemy->entity.x + enemy->entity.width / 2 - 5;
            bullets[i].entity.y = enemy->entity.y + enemy->entity.height;
            bullets[i].entity.width = 10;
            bullets[i].entity.height = 10;
            bullets[i].entity.active = true;
            
            // Aim at player
            float dx = player->entity.x + player->entity.width / 2 - bullets[i].entity.x;
            float dy = player->entity.y + player->entity.height / 2 - bullets[i].entity.y;
            float length = sqrt(dx * dx + dy * dy);
            
            bullets[i].dx = (dx / length) * (BULLET_SPEED * 0.7f);
            bullets[i].dy = (dy / length) * (BULLET_SPEED * 0.7f);
            bullets[i].damage = 1;
            break;
        }
    }
}

void updateBullets(Bullet bullets[], int maxBullets) {
    for (int i = 0; i < maxBullets; i++) {
        if (bullets[i].entity.active) {
            bullets[i].entity.x += bullets[i].dx;
            bullets[i].entity.y += bullets[i].dy;
            
            // Remove if off screen
            if (bullets[i].entity.y < -bullets[i].entity.height || 
                bullets[i].entity.y > WINDOW_HEIGHT ||
                bullets[i].entity.x < -bullets[i].entity.width ||
                bullets[i].entity.x > WINDOW_WIDTH) {
                bullets[i].entity.active = false;
            }
        }
    }
}

void checkCollisions(Player* player, Enemy enemies[], int maxEnemies, Bullet playerBullets[], int maxPlayerBullets, Bullet enemyBullets[], int maxEnemyBullets) {
    // Check player bullets vs enemies
    for (int i = 0; i < maxPlayerBullets; i++) {
        if (playerBullets[i].entity.active) {
            for (int j = 0; j < maxEnemies; j++) {
                if (enemies[j].entity.active) {
                    if (playerBullets[i].entity.x < enemies[j].entity.x + enemies[j].entity.width &&
                        playerBullets[i].entity.x + playerBullets[i].entity.width > enemies[j].entity.x &&
                        playerBullets[i].entity.y < enemies[j].entity.y + enemies[j].entity.height &&
                        playerBullets[i].entity.y + playerBullets[i].entity.height > enemies[j].entity.y) {
                        
                        // Collision detected
                        enemies[j].health -= playerBullets[i].damage;
                        playerBullets[i].entity.active = false;
                        
                        if (enemies[j].health <= 0) {
                            enemies[j].entity.active = false;
                            player->score += 100;
                        }
                        
                        break;
                    }
                }
            }
        }
    }
    
    // Check enemy bullets vs player
    if (player->entity.active && player->invincibility <= 0) {
        for (int i = 0; i < maxEnemyBullets; i++) {
            if (enemyBullets[i].entity.active) {
                if (enemyBullets[i].entity.x < player->entity.x + player->entity.width &&
                    enemyBullets[i].entity.x + enemyBullets[i].entity.width > player->entity.x &&
                    enemyBullets[i].entity.y < player->entity.y + player->entity.height &&
                    enemyBullets[i].entity.y + enemyBullets[i].entity.height > player->entity.y) {
                    
                    // Collision detected
                    player->health--;
                    enemyBullets[i].entity.active = false;
                    player->invincibility = 60; // 1 second invincibility
                    
                    if (player->health <= 0) {
                        player->entity.active = false;
                        printf("Game Over! Final Score: %d\n", player->score);
                    }
                }
            }
        }
    }
    
    // Check enemies vs player
    if (player->entity.active && player->invincibility <= 0) {
        for (int i = 0; i < maxEnemies; i++) {
            if (enemies[i].entity.active) {
                if (enemies[i].entity.x < player->entity.x + player->entity.width &&
                    enemies[i].entity.x + enemies[i].entity.width > player->entity.x &&
                    enemies[i].entity.y < player->entity.y + player->entity.height &&
                    enemies[i].entity.y + enemies[i].entity.height > player->entity.y) {
                    
                    // Collision detected
                    player->health--;
                    enemies[i].entity.active = false;
                    player->invincibility = 60; // 1 second invincibility
                    
                    if (player->health <= 0) {
                        player->entity.active = false;
                        printf("Game Over! Final Score: %d\n", player->score);
                    }
                }
            }
        }
    }
}

void updateBackground(Background* background) {
    background->y += background->speed;
    
    if (background->y >= WINDOW_HEIGHT) {
        background->y = 0;
    }
}

void renderGame(SDL_Renderer* renderer, SDL_Texture* textures[], Player* player, Enemy enemies[], int maxEnemies, Bullet playerBullets[], int maxPlayerBullets, Bullet enemyBullets[], int maxEnemyBullets, Background* background) {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Render background
    SDL_Rect bgRect1 = {0, background->y, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_Rect bgRect2 = {0, background->y - WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderCopy(renderer, textures[TEXTURE_BACKGROUND], NULL, &bgRect1);
    SDL_RenderCopy(renderer, textures[TEXTURE_BACKGROUND], NULL, &bgRect2);
    
    // Render player
    if (player->entity.active) {
        // Flash when invincible
        if (player->invincibility == 0 || (player->invincibility / 5) % 2 == 0) {
            SDL_Rect playerRect = {(int)player->entity.x, (int)player->entity.y, player->entity.width, player->entity.height};
            SDL_RenderCopy(renderer, textures[TEXTURE_PLAYER], NULL, &playerRect);
        }
    }
    
    // Render enemies
    for (int i = 0; i < maxEnemies; i++) {
        if (enemies[i].entity.active) {
            SDL_Rect enemyRect = {(int)enemies[i].entity.x, (int)enemies[i].entity.y, enemies[i].entity.width, enemies[i].entity.height};
            SDL_RenderCopy(renderer, textures[enemies[i].type == 0 ? TEXTURE_ENEMY1 : TEXTURE_ENEMY2], NULL, &enemyRect);
        }
    }
    
    // Render player bullets
    for (int i = 0; i < maxPlayerBullets; i++) {
        if (playerBullets[i].entity.active) {
            SDL_Rect bulletRect = {(int)playerBullets[i].entity.x, (int)playerBullets[i].entity.y, playerBullets[i].entity.width, playerBullets[i].entity.height};
            SDL_RenderCopy(renderer, textures[TEXTURE_PLAYER_BULLET], NULL, &bulletRect);
        }
    }
    
    // Render enemy bullets
    for (int i = 0; i < maxEnemyBullets; i++) {
        if (enemyBullets[i].entity.active) {
            SDL_Rect bulletRect = {(int)enemyBullets[i].entity.x, (int)enemyBullets[i].entity.y, enemyBullets[i].entity.width, enemyBullets[i].entity.height};
            SDL_RenderCopy(renderer, textures[TEXTURE_ENEMY_BULLET], NULL, &bulletRect);
        }
    }
    
    // Present the rendered frame
    SDL_RenderPresent(renderer);
}
