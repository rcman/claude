/**
 * Spy Hunter Game Clone using SDL2
 * 
 * A simplified version of the classic arcade game Spy Hunter.
 * Player controls a spy car, dodging and shooting enemy vehicles.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Game constants
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 800
#define FPS 60
#define FRAME_DELAY (1000 / FPS)

#define PLAYER_WIDTH 50
#define PLAYER_HEIGHT 80
#define PLAYER_SPEED 5
#define BULLET_WIDTH 8
#define BULLET_HEIGHT 16
#define BULLET_SPEED 10
#define MAX_BULLETS 10
#define ENEMY_WIDTH 50
#define ENEMY_HEIGHT 80
#define ENEMY_MIN_SPEED 2
#define ENEMY_MAX_SPEED 4
#define MAX_ENEMIES 8
#define ROAD_SPEED 5
#define ROAD_SEGMENT_HEIGHT 200
#define MAX_ROAD_SEGMENTS 5
#define ROAD_WIDTH 400
#define GRASS_WIDTH ((SCREEN_WIDTH - ROAD_WIDTH) / 2)
#define SPAWN_INTERVAL 2000 // milliseconds

// Game object structures
typedef struct {
    float x, y;
    float speed;
    bool active;
    SDL_Rect rect;
} Bullet;

typedef struct {
    float x, y;
    float speed;
    bool active;
    bool civilian; // true for civilian cars, false for enemy cars
    SDL_Rect rect;
} Enemy;

typedef struct {
    float x, y;
    SDL_Rect rect;
} RoadSegment;

// Game state
typedef struct {
    // Player
    float playerX, playerY;
    SDL_Rect playerRect;
    
    // Bullets
    Bullet bullets[MAX_BULLETS];
    int bulletCount;
    
    // Enemies
    Enemy enemies[MAX_ENEMIES];
    int enemyCount;
    Uint32 lastEnemySpawn;
    
    // Road
    RoadSegment roadSegments[MAX_ROAD_SEGMENTS];
    
    // Score
    int score;
    
    // Game state
    bool running;
    bool gameOver;
} GameState;

// Function prototypes
bool initSDL(SDL_Window** window, SDL_Renderer** renderer);
void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer);
void initGame(GameState* game);
void handleInput(GameState* game, SDL_Event* event);
void updateGame(GameState* game);
void renderGame(SDL_Renderer* renderer, GameState* game);
void spawnEnemy(GameState* game);
void fireBullet(GameState* game);
bool checkCollision(SDL_Rect* a, SDL_Rect* b);

int main(int argc, char* argv[]) {
    // Initialize random number generator
    srand(time(NULL));
    
    // Setup SDL
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    
    if (!initSDL(&window, &renderer)) {
        return 1;
    }
    
    // Initialize game state
    GameState game;
    initGame(&game);
    
    // Game loop variables
    SDL_Event event;
    Uint32 frameStart, frameTime;
    
    // Game loop
    while (game.running) {
        frameStart = SDL_GetTicks();
        
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game.running = false;
            }
            
            handleInput(&game, &event);
        }
        
        // Update game state
        updateGame(&game);
        
        // Render game
        renderGame(renderer, &game);
        
        // Cap frame rate
        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }
    
    // Cleanup
    cleanupSDL(window, renderer);
    
    return 0;
}

bool initSDL(SDL_Window** window, SDL_Renderer** renderer) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Create window
    *window = SDL_CreateWindow("Spy Hunter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Create renderer
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }
    
    return true;
}

void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void initGame(GameState* game) {
    // Initialize player
    game->playerX = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
    game->playerY = SCREEN_HEIGHT - PLAYER_HEIGHT - 20;
    game->playerRect.x = (int)game->playerX;
    game->playerRect.y = (int)game->playerY;
    game->playerRect.w = PLAYER_WIDTH;
    game->playerRect.h = PLAYER_HEIGHT;
    
    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        game->bullets[i].active = false;
    }
    game->bulletCount = 0;
    
    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i].active = false;
    }
    game->enemyCount = 0;
    game->lastEnemySpawn = 0;
    
    // Initialize road segments
    for (int i = 0; i < MAX_ROAD_SEGMENTS; i++) {
        game->roadSegments[i].y = i * ROAD_SEGMENT_HEIGHT - ROAD_SEGMENT_HEIGHT;
        game->roadSegments[i].rect.x = GRASS_WIDTH;
        game->roadSegments[i].rect.y = (int)game->roadSegments[i].y;
        game->roadSegments[i].rect.w = ROAD_WIDTH;
        game->roadSegments[i].rect.h = ROAD_SEGMENT_HEIGHT;
    }
    
    // Initialize score
    game->score = 0;
    
    // Initialize game state
    game->running = true;
    game->gameOver = false;
}

void handleInput(GameState* game, SDL_Event* event) {
    // Handle key press events
    if (event->type == SDL_KEYDOWN && !event->key.repeat) {
        switch (event->key.keysym.sym) {
            case SDLK_SPACE:
                fireBullet(game);
                break;
            case SDLK_r:
                if (game->gameOver) {
                    initGame(game);
                }
                break;
            case SDLK_ESCAPE:
                game->running = false;
                break;
        }
    }
}

void updateGame(GameState* game) {
    if (game->gameOver) {
        return;
    }
    
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    
    // Update player position based on keyboard input
    if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {
        game->playerX -= PLAYER_SPEED;
    }
    if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
        game->playerX += PLAYER_SPEED;
    }
    if (keystates[SDL_SCANCODE_UP] || keystates[SDL_SCANCODE_W]) {
        game->playerY -= PLAYER_SPEED / 2;
    }
    if (keystates[SDL_SCANCODE_DOWN] || keystates[SDL_SCANCODE_S]) {
        game->playerY += PLAYER_SPEED / 2;
    }
    
    // Keep player within screen bounds
    if (game->playerX < GRASS_WIDTH) {
        game->playerX = GRASS_WIDTH;
    }
    if (game->playerX > SCREEN_WIDTH - GRASS_WIDTH - PLAYER_WIDTH) {
        game->playerX = SCREEN_WIDTH - GRASS_WIDTH - PLAYER_WIDTH;
    }
    if (game->playerY < 0) {
        game->playerY = 0;
    }
    if (game->playerY > SCREEN_HEIGHT - PLAYER_HEIGHT) {
        game->playerY = SCREEN_HEIGHT - PLAYER_HEIGHT;
    }
    
    // Update player rectangle
    game->playerRect.x = (int)game->playerX;
    game->playerRect.y = (int)game->playerY;
    
    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            game->bullets[i].y -= game->bullets[i].speed;
            game->bullets[i].rect.y = (int)game->bullets[i].y;
            
            // Deactivate bullets that go off-screen
            if (game->bullets[i].y + BULLET_HEIGHT < 0) {
                game->bullets[i].active = false;
                game->bulletCount--;
            }
        }
    }
    
    // Spawn enemies
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - game->lastEnemySpawn > SPAWN_INTERVAL) {
        spawnEnemy(game);
        game->lastEnemySpawn = currentTime;
    }
    
    // Update enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            game->enemies[i].y += game->enemies[i].speed;
            game->enemies[i].rect.y = (int)game->enemies[i].y;
            
            // Deactivate enemies that go off-screen
            if (game->enemies[i].y > SCREEN_HEIGHT) {
                game->enemies[i].active = false;
                game->enemyCount--;
            }
            
            // Check for collision with player
            if (checkCollision(&game->playerRect, &game->enemies[i].rect)) {
                if (!game->enemies[i].civilian) {
                    game->gameOver = true;
                }
            }
            
            // Check for collision with bullets
            for (int j = 0; j < MAX_BULLETS; j++) {
                if (game->bullets[j].active && checkCollision(&game->bullets[j].rect, &game->enemies[i].rect)) {
                    game->bullets[j].active = false;
                    game->bulletCount--;
                    
                    if (!game->enemies[i].civilian) {
                        game->score += 100;
                        game->enemies[i].active = false;
                        game->enemyCount--;
                    } else {
                        // Penalty for shooting civilian cars
                        game->score -= 200;
                    }
                }
            }
        }
    }
    
    // Update road segments
    for (int i = 0; i < MAX_ROAD_SEGMENTS; i++) {
        game->roadSegments[i].y += ROAD_SPEED;
        game->roadSegments[i].rect.y = (int)game->roadSegments[i].y;
        
        // Reset road segment when it goes off-screen
        if (game->roadSegments[i].y >= SCREEN_HEIGHT) {
            game->roadSegments[i].y = -ROAD_SEGMENT_HEIGHT + (game->roadSegments[i].y - SCREEN_HEIGHT);
            game->roadSegments[i].rect.y = (int)game->roadSegments[i].y;
        }
    }
}

void renderGame(SDL_Renderer* renderer, GameState* game) {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Render road segments and grass
    for (int i = 0; i < MAX_ROAD_SEGMENTS; i++) {
        // Left grass
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
        SDL_Rect grassLeftRect = {0, game->roadSegments[i].rect.y, GRASS_WIDTH, ROAD_SEGMENT_HEIGHT};
        SDL_RenderFillRect(renderer, &grassLeftRect);
        
        // Right grass
        SDL_Rect grassRightRect = {SCREEN_WIDTH - GRASS_WIDTH, game->roadSegments[i].rect.y, GRASS_WIDTH, ROAD_SEGMENT_HEIGHT};
        SDL_RenderFillRect(renderer, &grassRightRect);
        
        // Road
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &game->roadSegments[i].rect);
        
        // Road markings
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int roadCenter = GRASS_WIDTH + ROAD_WIDTH / 2;
        SDL_Rect lineRect = {roadCenter - 5, game->roadSegments[i].rect.y, 10, ROAD_SEGMENT_HEIGHT / 5};
        
        for (int j = 0; j < 5; j++) {
            if (j % 2 == 0) {
                SDL_RenderFillRect(renderer, &lineRect);
            }
            lineRect.y += ROAD_SEGMENT_HEIGHT / 5;
        }
    }
    
    // Render bullets
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            SDL_RenderFillRect(renderer, &game->bullets[i].rect);
        }
    }
    
    // Render enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            if (game->enemies[i].civilian) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Civilian cars are green
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Enemy cars are red
            }
            SDL_RenderFillRect(renderer, &game->enemies[i].rect);
        }
    }
    
    // Render player
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRect(renderer, &game->playerRect);
    
    // Render game over message if needed
    if (game->gameOver) {
        // This is a simple representation - in a real game you'd use SDL_ttf for text
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect gameOverRect = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 3, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 6};
        SDL_RenderFillRect(renderer, &gameOverRect);
    }
    
    // Update screen
    SDL_RenderPresent(renderer);
}

void spawnEnemy(GameState* game) {
    if (game->enemyCount >= MAX_ENEMIES) {
        return;
    }
    
    // Find an inactive enemy slot
    int index = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!game->enemies[i].active) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return;
    }
    
    // Initialize the enemy
    game->enemies[index].active = true;
    
    // Randomly position on the road
    int roadMin = GRASS_WIDTH + ENEMY_WIDTH / 2;
    int roadMax = SCREEN_WIDTH - GRASS_WIDTH - ENEMY_WIDTH;
    game->enemies[index].x = roadMin + rand() % (roadMax - roadMin);
    game->enemies[index].y = -ENEMY_HEIGHT;
    
    // Set speed
    game->enemies[index].speed = ENEMY_MIN_SPEED + (float)rand() / RAND_MAX * (ENEMY_MAX_SPEED - ENEMY_MIN_SPEED);
    
    // 30% chance for civilian car
    game->enemies[index].civilian = (rand() % 100) < 30;
    
    // Set rectangle
    game->enemies[index].rect.x = (int)game->enemies[index].x;
    game->enemies[index].rect.y = (int)game->enemies[index].y;
    game->enemies[index].rect.w = ENEMY_WIDTH;
    game->enemies[index].rect.h = ENEMY_HEIGHT;
    
    game->enemyCount++;
}

void fireBullet(GameState* game) {
    if (game->bulletCount >= MAX_BULLETS || game->gameOver) {
        return;
    }
    
    // Find an inactive bullet slot
    int index = -1;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!game->bullets[i].active) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return;
    }
    
    // Initialize the bullet
    game->bullets[index].active = true;
    game->bullets[index].x = game->playerX + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
    game->bullets[index].y = game->playerY - BULLET_HEIGHT;
    game->bullets[index].speed = BULLET_SPEED;
    
    // Set rectangle
    game->bullets[index].rect.x = (int)game->bullets[index].x;
    game->bullets[index].rect.y = (int)game->bullets[index].y;
    game->bullets[index].rect.w = BULLET_WIDTH;
    game->bullets[index].rect.h = BULLET_HEIGHT;
    
    game->bulletCount++;
}

bool checkCollision(SDL_Rect* a, SDL_Rect* b) {
    // Check if two rectangles overlap
    return (a->x < b->x + b->w &&
            a->x + a->w > b->x &&
            a->y < b->y + b->h &&
            a->y + a->h > b->y);
}
