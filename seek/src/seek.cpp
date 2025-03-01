#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Game constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define TERRAIN_HEIGHT 400
#define PLAYER_WIDTH 64
#define PLAYER_HEIGHT 32
#define BULLET_WIDTH 8
#define BULLET_HEIGHT 4
#define ENEMY_WIDTH 32
#define ENEMY_HEIGHT 32
#define BUILDING_WIDTH 48
#define BUILDING_HEIGHT 48
#define MAX_BULLETS 20
#define MAX_ENEMIES 10
#define MAX_BUILDINGS 15
#define PLAYER_SPEED 5
#define BULLET_SPEED 10
#define ENEMY_SPEED 2
#define GRAVITY 0.5
#define TERRAIN_SEGMENTS 100
#define SEGMENT_WIDTH (WINDOW_WIDTH / TERRAIN_SEGMENTS)

typedef struct {
    float x, y;
    float vx, vy;
    bool active;
} Bullet;

typedef struct {
    float x, y;
    float vx, vy;
    bool active;
    int health;
    int type; // 0 for ground unit, 1 for air unit
} Enemy;

typedef struct {
    float x, y;
    int width, height;
    bool active;
    int health;
} Building;

typedef struct {
    float x, y;
    float vx, vy;
    int health;
    bool firing;
    int score;
    int ammo;
    int fuel;
} Player;

typedef struct {
    int heights[TERRAIN_SEGMENTS + 1];
    int segments;
} Terrain;

// Function prototypes
bool initialize(SDL_Window** window, SDL_Renderer** renderer);
void cleanup(SDL_Window* window, SDL_Renderer* renderer);
void handleInput(SDL_Event* event, bool* quit, Player* player);
void updateGame(Player* player, Bullet bullets[], Enemy enemies[], Building buildings[], Terrain* terrain);
void renderGame(SDL_Renderer* renderer, Player player, Bullet bullets[], Enemy enemies[], Building buildings[], Terrain terrain);
void generateTerrain(Terrain* terrain);
void spawnEnemy(Enemy enemies[]);
void spawnBuilding(Building buildings[], Terrain terrain);
bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2);
int getTerrainHeight(float x, Terrain terrain);

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event event;
    bool quit = false;
    Player player = {100, 100, 0, 0, 100, false, 0, 100, 1000};
    Bullet bullets[MAX_BULLETS] = {0};
    Enemy enemies[MAX_ENEMIES] = {0};
    Building buildings[MAX_BUILDINGS] = {0};
    Terrain terrain = {0};
    
    srand(time(NULL));
    
    if (!initialize(&window, &renderer)) {
        return 1;
    }
    
    generateTerrain(&terrain);
    
    // Initialize some enemies and buildings
    for (int i = 0; i < 5; i++) {
        spawnEnemy(enemies);
    }
    
    for (int i = 0; i < 8; i++) {
        spawnBuilding(buildings, terrain);
    }
    
    Uint32 lastTime = SDL_GetTicks();
    
    // Game loop
    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // Handle events
        while (SDL_PollEvent(&event)) {
            handleInput(&event, &quit, &player);
        }
        
        // Update game state
        updateGame(&player, bullets, enemies, buildings, &terrain);
        
        // Render the game
        renderGame(renderer, player, bullets, enemies, buildings, terrain);
        
        // Cap the frame rate
        SDL_Delay(16); // ~60 FPS
        
        // Decrease fuel over time
        player.fuel -= 1;
        
        // Game over condition
        if (player.health <= 0 || player.fuel <= 0) {
            printf("Game Over! Final Score: %d\n", player.score);
            quit = true;
        }
        
        // Spawn enemies occasionally
        if (rand() % 100 < 2) {
            spawnEnemy(enemies);
        }
    }
    
    cleanup(window, renderer);
    return 0;
}

bool initialize(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    *window = SDL_CreateWindow("Seek and Destroy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if(!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }
    
    return true;
}

void cleanup(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void handleInput(SDL_Event* event, bool* quit, Player* player) {
    if (event->type == SDL_QUIT) {
        *quit = true;
    } else if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_ESCAPE:
                *quit = true;
                break;
            case SDLK_SPACE:
                player->firing = true;
                break;
        }
    } else if (event->type == SDL_KEYUP) {
        switch (event->key.keysym.sym) {
            case SDLK_SPACE:
                player->firing = false;
                break;
        }
    }
    
    // Continuous key state handling
    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    
    // Reset velocity
    player->vx = 0;
    player->vy = 0;
    
    if (keyState[SDL_SCANCODE_UP]) {
        player->vy = -PLAYER_SPEED;
    }
    if (keyState[SDL_SCANCODE_DOWN]) {
        player->vy = PLAYER_SPEED;
    }
    if (keyState[SDL_SCANCODE_LEFT]) {
        player->vx = -PLAYER_SPEED;
    }
    if (keyState[SDL_SCANCODE_RIGHT]) {
        player->vx = PLAYER_SPEED;
    }
}

void updateGame(Player* player, Bullet bullets[], Enemy enemies[], Building buildings[], Terrain* terrain) {
    // Update player position
    player->x += player->vx;
    player->y += player->vy;
    
    // Keep player within bounds
    if (player->x < 0) player->x = 0;
    if (player->x > WINDOW_WIDTH - PLAYER_WIDTH) player->x = WINDOW_WIDTH - PLAYER_WIDTH;
    if (player->y < 0) player->y = 0;
    if (player->y > WINDOW_HEIGHT - PLAYER_HEIGHT) player->y = WINDOW_HEIGHT - PLAYER_HEIGHT;
    
    // Check player collision with terrain
    int terrainHeightAtPlayer = getTerrainHeight(player->x + PLAYER_WIDTH / 2, *terrain);
    if (player->y + PLAYER_HEIGHT > WINDOW_HEIGHT - terrainHeightAtPlayer) {
        player->health = 0; // Crash!
    }
    
    // Fire bullet if player is firing and has ammo
    static int fireDelay = 0;
    if (player->firing && player->ammo > 0 && fireDelay == 0) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].x = player->x + PLAYER_WIDTH;
                bullets[i].y = player->y + PLAYER_HEIGHT / 2;
                bullets[i].vx = BULLET_SPEED;
                bullets[i].vy = 0;
                bullets[i].active = true;
                player->ammo--;
                fireDelay = 10; // Delay between shots
                break;
            }
        }
    }
    
    if (fireDelay > 0) fireDelay--;
    
    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].vx;
            bullets[i].y += bullets[i].vy;
            
            // Check if bullet is out of bounds
            if (bullets[i].x < 0 || bullets[i].x > WINDOW_WIDTH || 
                bullets[i].y < 0 || bullets[i].y > WINDOW_HEIGHT) {
                bullets[i].active = false;
                continue;
            }
            
            // Check bullet collision with terrain
            int terrainHeightAtBullet = getTerrainHeight(bullets[i].x, *terrain);
            if (bullets[i].y + BULLET_HEIGHT > WINDOW_HEIGHT - terrainHeightAtBullet) {
                bullets[i].active = false;
                continue;
            }
            
            // Check bullet collision with enemies
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].active && 
                    checkCollision(bullets[i].x, bullets[i].y, BULLET_WIDTH, BULLET_HEIGHT,
                                 enemies[j].x, enemies[j].y, ENEMY_WIDTH, ENEMY_HEIGHT)) {
                    bullets[i].active = false;
                    enemies[j].health -= 25;
                    if (enemies[j].health <= 0) {
                        enemies[j].active = false;
                        player->score += 100;
                        player->ammo += 10; // Ammo bonus for killing enemy
                    }
                    break;
                }
            }
            
            // Check bullet collision with buildings
            for (int j = 0; j < MAX_BUILDINGS; j++) {
                if (buildings[j].active && 
                    checkCollision(bullets[i].x, bullets[i].y, BULLET_WIDTH, BULLET_HEIGHT,
                                 buildings[j].x, buildings[j].y, buildings[j].width, buildings[j].height)) {
                    bullets[i].active = false;
                    buildings[j].health -= 25;
                    if (buildings[j].health <= 0) {
                        buildings[j].active = false;
                        player->score += 50;
                    }
                    break;
                }
            }
        }
    }
    
    // Update enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            // Move towards player
            float dx = player->x - enemies[i].x;
            float dy = player->y - enemies[i].y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance > 0) {
                enemies[i].vx = dx / distance * ENEMY_SPEED;
                enemies[i].vy = dy / distance * ENEMY_SPEED;
            }
            
            enemies[i].x += enemies[i].vx;
            enemies[i].y += enemies[i].vy;
            
            // Keep ground units on the ground
            if (enemies[i].type == 0) {
                int terrainHeight = getTerrainHeight(enemies[i].x + ENEMY_WIDTH / 2, *terrain);
                enemies[i].y = WINDOW_HEIGHT - terrainHeight - ENEMY_HEIGHT;
            }
            
            // Check if enemy is out of bounds
            if (enemies[i].x < 0 || enemies[i].x > WINDOW_WIDTH || 
                enemies[i].y < 0 || enemies[i].y > WINDOW_HEIGHT) {
                enemies[i].active = false;
                continue;
            }
            
            // Check enemy collision with player
            if (checkCollision(player->x, player->y, PLAYER_WIDTH, PLAYER_HEIGHT,
                             enemies[i].x, enemies[i].y, ENEMY_WIDTH, ENEMY_HEIGHT)) {
                player->health -= 10;
                enemies[i].health -= 50;
                if (enemies[i].health <= 0) {
                    enemies[i].active = false;
                    player->score += 100;
                }
            }
            
            // Enemy occasionally fires at player (for air units)
            if (enemies[i].type == 1 && rand() % 100 < 1) {
                for (int j = 0; j < MAX_BULLETS; j++) {
                    if (!bullets[j].active) {
                        bullets[j].x = enemies[i].x;
                        bullets[j].y = enemies[i].y + ENEMY_HEIGHT / 2;
                        
                        // Calculate direction towards player
                        float dx = player->x - enemies[i].x;
                        float dy = player->y - enemies[i].y;
                        float distance = sqrt(dx * dx + dy * dy);
                        
                        if (distance > 0) {
                            bullets[j].vx = dx / distance * BULLET_SPEED;
                            bullets[j].vy = dy / distance * BULLET_SPEED;
                        } else {
                            bullets[j].vx = -BULLET_SPEED;
                            bullets[j].vy = 0;
                        }
                        
                        bullets[j].active = true;
                        break;
                    }
                }
            }
        }
    }
    
    // Occasionally add fuel
    if (rand() % 1000 < 5) {
        player->fuel += 100;
        if (player->fuel > 1000) player->fuel = 1000;
    }
}

void renderGame(SDL_Renderer* renderer, Player player, Bullet bullets[], Enemy enemies[], Building buildings[], Terrain terrain) {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky blue
    SDL_RenderClear(renderer);
    
    // Draw terrain
    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255); // Forest green
    for (int i = 0; i < terrain.segments; i++) {
        SDL_Rect terrainRect = {
            i * SEGMENT_WIDTH,
            WINDOW_HEIGHT - terrain.heights[i],
            SEGMENT_WIDTH + 1, // +1 to avoid gaps
            terrain.heights[i]
        };
        SDL_RenderFillRect(renderer, &terrainRect);
    }
    
    // Draw buildings
    SDL_SetRenderDrawColor(renderer, 169, 169, 169, 255); // Dark gray
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (buildings[i].active) {
            SDL_Rect buildingRect = {
                (int)buildings[i].x,
                (int)buildings[i].y,
                buildings[i].width,
                buildings[i].height
            };
            SDL_RenderFillRect(renderer, &buildingRect);
        }
    }
    
    // Draw bullets
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            SDL_Rect bulletRect = {
                (int)bullets[i].x,
                (int)bullets[i].y,
                BULLET_WIDTH,
                BULLET_HEIGHT
            };
            SDL_RenderFillRect(renderer, &bulletRect);
        }
    }
    
    // Draw enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            if (enemies[i].type == 0) {
                SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Brown (ground)
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red (air)
            }
            
            SDL_Rect enemyRect = {
                (int)enemies[i].x,
                (int)enemies[i].y,
                ENEMY_WIDTH,
                ENEMY_HEIGHT
            };
            SDL_RenderFillRect(renderer, &enemyRect);
        }
    }
    
    // Draw player helicopter
    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); // Dark green
    SDL_Rect playerRect = {
        (int)player.x,
        (int)player.y,
        PLAYER_WIDTH,
        PLAYER_HEIGHT
    };
    SDL_RenderFillRect(renderer, &playerRect);
    
    // Draw rotor
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect rotorRect = {
        (int)player.x + PLAYER_WIDTH / 4,
        (int)player.y - 5,
        PLAYER_WIDTH / 2,
        5
    };
    SDL_RenderFillRect(renderer, &rotorRect);
    
    // Draw HUD
    char scoreText[32];
    char healthText[32];
    char ammoText[32];
    char fuelText[32];
    
    sprintf(scoreText, "Score: %d", player.score);
    sprintf(healthText, "Health: %d", player.health);
    sprintf(ammoText, "Ammo: %d", player.ammo);
    sprintf(fuelText, "Fuel: %d", player.fuel);
    
    // We would use SDL_ttf to render text, but for simplicity we'll just use rectangles as placeholders
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_Rect hudRect = {10, 10, 200, 100};
    SDL_RenderFillRect(renderer, &hudRect);
    
    // Present render
    SDL_RenderPresent(renderer);
}

void generateTerrain(Terrain* terrain) {
    terrain->segments = TERRAIN_SEGMENTS;
    
    // Generate a simple hilly terrain
    int baseHeight = TERRAIN_HEIGHT / 4;
    int prevHeight = baseHeight;
    
    for (int i = 0; i <= terrain->segments; i++) {
        int newHeight = prevHeight + (rand() % 21 - 10);
        
        // Keep within bounds
        if (newHeight < baseHeight / 2) newHeight = baseHeight / 2;
        if (newHeight > baseHeight * 2) newHeight = baseHeight * 2;
        
        terrain->heights[i] = newHeight;
        prevHeight = newHeight;
    }
}

void spawnEnemy(Enemy enemies[]) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].active = true;
            enemies[i].health = 100;
            enemies[i].type = rand() % 2; // 0 for ground, 1 for air
            
            // Spawn off-screen
            enemies[i].x = WINDOW_WIDTH;
            
            if (enemies[i].type == 1) {
                // Air unit - random height
                enemies[i].y = rand() % (WINDOW_HEIGHT / 2);
            } else {
                // Ground unit - on the ground
                enemies[i].y = WINDOW_HEIGHT - TERRAIN_HEIGHT / 2 - ENEMY_HEIGHT;
            }
            
            enemies[i].vx = -ENEMY_SPEED;
            enemies[i].vy = 0;
            
            break;
        }
    }
}

void spawnBuilding(Building buildings[], Terrain terrain) {
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        if (!buildings[i].active) {
            buildings[i].active = true;
            buildings[i].health = 100;
            
            // Random x position
            buildings[i].x = rand() % (WINDOW_WIDTH - BUILDING_WIDTH);
            
            // Place on terrain
            int terrainHeight = getTerrainHeight(buildings[i].x + BUILDING_WIDTH / 2, terrain);
            buildings[i].y = WINDOW_HEIGHT - terrainHeight - BUILDING_HEIGHT;
            
            // Random size variation
            buildings[i].width = BUILDING_WIDTH + (rand() % 20 - 10);
            buildings[i].height = BUILDING_HEIGHT + (rand() % 20 - 10);
            
            break;
        }
    }
}

bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

int getTerrainHeight(float x, Terrain terrain) {
    if (x < 0) return terrain.heights[0];
    if (x >= WINDOW_WIDTH) return terrain.heights[terrain.segments];
    
    int segment = (int)(x / SEGMENT_WIDTH);
    float t = (x - segment * SEGMENT_WIDTH) / SEGMENT_WIDTH;
    
    // Linear interpolation between segments
    return terrain.heights[segment] * (1 - t) + terrain.heights[segment + 1] * t;
}
