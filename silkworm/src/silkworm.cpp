#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Game constants
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SPEED 5
#define BULLET_SPEED 10
#define MAX_BULLETS 20
#define MAX_ENEMIES 10
#define ENEMY_SPEED 3
#define SCROLL_SPEED 2

// Game states
typedef enum {
    GAME_RUNNING,
    GAME_OVER
} GameState;

// Entity types
typedef enum {
    ENTITY_PLAYER,
    ENTITY_ENEMY,
    ENTITY_BULLET,
    ENTITY_HELICOPTER, // Player 1
    ENTITY_JEEP        // Player 2
} EntityType;

// Entity structure
typedef struct {
    float x, y;
    int width, height;
    bool active;
    EntityType type;
    int health;
    float velocity_x, velocity_y;
} Entity;

// Game structure
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* background;
    SDL_Texture* helicopter;
    SDL_Texture* jeep;
    SDL_Texture* enemy;
    SDL_Texture* bullet;
    Entity player_helicopter;
    Entity player_jeep;
    Entity bullets[MAX_BULLETS];
    Entity enemies[MAX_ENEMIES];
    int bg_scroll_offset;
    Uint32 last_enemy_spawn;
    GameState state;
    int score;
} Game;

// Function prototypes
bool initialize_game(Game* game);
void clean_up(Game* game);
void handle_input(Game* game, bool* quit);
void update_game(Game* game);
void render_game(Game* game);
void spawn_enemy(Game* game);
void spawn_bullet(Game* game, Entity* player);
bool check_collision(Entity* a, Entity* b);

int main(int argc, char* argv[]) {
    Game game;
    bool quit = false;
    Uint32 frame_start;
    int frame_time;

    // Seed random number generator
    srand(time(NULL));

    // Initialize game
    if (!initialize_game(&game)) {
        return 1;
    }

    // Game loop
    while (!quit && game.state != GAME_OVER) {
        frame_start = SDL_GetTicks();

        handle_input(&game, &quit);
        update_game(&game);
        render_game(&game);

        // Cap to 60 FPS
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time);
        }
    }

    // Clean up
    clean_up(&game);
    return 0;
}

bool initialize_game(Game* game) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }

    // Create window
    game->window = SDL_CreateWindow("Silkworm Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (game->window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Create renderer
    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (game->renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Load textures (In a real game, you would load actual image files)
    // We'll create colored rectangles for demonstration purposes
    
    // Create background texture
    game->background = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT * 2);
    SDL_SetRenderTarget(game->renderer, game->background);
    SDL_SetRenderDrawColor(game->renderer, 50, 100, 50, 255); // Green background
    SDL_RenderClear(game->renderer);
    
    // Draw some background elements (trees, etc.)
    SDL_SetRenderDrawColor(game->renderer, 30, 80, 30, 255);
    for (int i = 0; i < 50; i++) {
        SDL_Rect rect = {rand() % SCREEN_WIDTH, rand() % (SCREEN_HEIGHT * 2), 20, 40};
        SDL_RenderFillRect(game->renderer, &rect);
    }
    
    SDL_SetRenderTarget(game->renderer, NULL);
    
    // Create helicopter texture
    game->helicopter = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET, 50, 30);
    SDL_SetRenderTarget(game->renderer, game->helicopter);
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 255, 255); // Blue helicopter
    SDL_RenderClear(game->renderer);
    SDL_SetRenderTarget(game->renderer, NULL);
    
    // Create jeep texture
    game->jeep = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_RGBA8888,
                                 SDL_TEXTUREACCESS_TARGET, 50, 30);
    SDL_SetRenderTarget(game->renderer, game->jeep);
    SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255); // Red jeep
    SDL_RenderClear(game->renderer);
    SDL_SetRenderTarget(game->renderer, NULL);
    
    // Create enemy texture
    game->enemy = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_TARGET, 40, 40);
    SDL_SetRenderTarget(game->renderer, game->enemy);
    SDL_SetRenderDrawColor(game->renderer, 255, 0, 255, 255); // Purple enemy
    SDL_RenderClear(game->renderer);
    SDL_SetRenderTarget(game->renderer, NULL);
    
    // Create bullet texture
    game->bullet = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_TARGET, 10, 5);
    SDL_SetRenderTarget(game->renderer, game->bullet);
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255); // Yellow bullet
    SDL_RenderClear(game->renderer);
    SDL_SetRenderTarget(game->renderer, NULL);
    
    // Initialize helicopter player
    game->player_helicopter.x = 100;
    game->player_helicopter.y = 200;
    game->player_helicopter.width = 50;
    game->player_helicopter.height = 30;
    game->player_helicopter.active = true;
    game->player_helicopter.type = ENTITY_HELICOPTER;
    game->player_helicopter.health = 3;
    
    // Initialize jeep player
    game->player_jeep.x = 100;
    game->player_jeep.y = SCREEN_HEIGHT - 80;
    game->player_jeep.width = 50;
    game->player_jeep.height = 30;
    game->player_jeep.active = true;
    game->player_jeep.type = ENTITY_JEEP;
    game->player_jeep.health = 3;
    
    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        game->bullets[i].active = false;
        game->bullets[i].type = ENTITY_BULLET;
        game->bullets[i].width = 10;
        game->bullets[i].height = 5;
    }
    
    // Initialize enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i].active = false;
        game->enemies[i].type = ENTITY_ENEMY;
        game->enemies[i].width = 40;
        game->enemies[i].height = 40;
        game->enemies[i].health = 1;
    }
    
    // Initialize game state
    game->bg_scroll_offset = 0;
    game->last_enemy_spawn = 0;
    game->state = GAME_RUNNING;
    game->score = 0;
    
    return true;
}

void clean_up(Game* game) {
    SDL_DestroyTexture(game->background);
    SDL_DestroyTexture(game->helicopter);
    SDL_DestroyTexture(game->jeep);
    SDL_DestroyTexture(game->enemy);
    SDL_DestroyTexture(game->bullet);
    SDL_DestroyRenderer(game->renderer);
    SDL_DestroyWindow(game->window);
    IMG_Quit();
    SDL_Quit();
}

void handle_input(Game* game, bool* quit) {
    SDL_Event event;
    
    // Handle events
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            *quit = true;
        }
    }
    
    // Handle keyboard input
    const Uint8* state = SDL_GetKeyboardState(NULL);
    
    // Helicopter controls
    if (state[SDL_SCANCODE_W]) {
        game->player_helicopter.y -= PLAYER_SPEED;
    }
    if (state[SDL_SCANCODE_S]) {
        game->player_helicopter.y += PLAYER_SPEED;
    }
    if (state[SDL_SCANCODE_A]) {
        game->player_helicopter.x -= PLAYER_SPEED;
    }
    if (state[SDL_SCANCODE_D]) {
        game->player_helicopter.x += PLAYER_SPEED;
    }
    if (state[SDL_SCANCODE_SPACE] && SDL_GetTicks() % 10 == 0) {
        spawn_bullet(game, &game->player_helicopter);
    }
    
    // Jeep controls
    if (state[SDL_SCANCODE_LEFT]) {
        game->player_jeep.x -= PLAYER_SPEED;
    }
    if (state[SDL_SCANCODE_RIGHT]) {
        game->player_jeep.x += PLAYER_SPEED;
    }
    if (state[SDL_SCANCODE_RETURN] && SDL_GetTicks() % 10 == 0) {
        spawn_bullet(game, &game->player_jeep);
    }
    
    // Keep players in bounds
    if (game->player_helicopter.x < 0) game->player_helicopter.x = 0;
    if (game->player_helicopter.x > SCREEN_WIDTH - game->player_helicopter.width) 
        game->player_helicopter.x = SCREEN_WIDTH - game->player_helicopter.width;
    if (game->player_helicopter.y < 0) game->player_helicopter.y = 0;
    if (game->player_helicopter.y > SCREEN_HEIGHT - game->player_helicopter.height) 
        game->player_helicopter.y = SCREEN_HEIGHT - game->player_helicopter.height;
    
    if (game->player_jeep.x < 0) game->player_jeep.x = 0;
    if (game->player_jeep.x > SCREEN_WIDTH - game->player_jeep.width) 
        game->player_jeep.x = SCREEN_WIDTH - game->player_jeep.width;
    
    // Keep jeep on the ground
    game->player_jeep.y = SCREEN_HEIGHT - 80;
}

void update_game(Game* game) {
    // Update background scroll
    game->bg_scroll_offset = (game->bg_scroll_offset + SCROLL_SPEED) % SCREEN_HEIGHT;
    
    // Spawn enemies
    if (SDL_GetTicks() - game->last_enemy_spawn > 1000) {
        spawn_enemy(game);
        game->last_enemy_spawn = SDL_GetTicks();
    }
    
    // Update bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            game->bullets[i].x += BULLET_SPEED;
            
            // Remove bullets that go off-screen
            if (game->bullets[i].x > SCREEN_WIDTH) {
                game->bullets[i].active = false;
            }
            
            // Check for collisions with enemies
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (game->enemies[j].active && check_collision(&game->bullets[i], &game->enemies[j])) {
                    game->enemies[j].health--;
                    game->bullets[i].active = false;
                    
                    if (game->enemies[j].health <= 0) {
                        game->enemies[j].active = false;
                        game->score += 100;
                    }
                    
                    break;
                }
            }
        }
    }
    
    // Update enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            game->enemies[i].x -= ENEMY_SPEED;
            
            // Remove enemies that go off-screen
            if (game->enemies[i].x + game->enemies[i].width < 0) {
                game->enemies[i].active = false;
            }
            
            // Check for collisions with players
            if (check_collision(&game->enemies[i], &game->player_helicopter)) {
                game->player_helicopter.health--;
                game->enemies[i].active = false;
                
                if (game->player_helicopter.health <= 0) {
                    game->player_helicopter.active = false;
                }
            }
            
            if (check_collision(&game->enemies[i], &game->player_jeep)) {
                game->player_jeep.health--;
                game->enemies[i].active = false;
                
                if (game->player_jeep.health <= 0) {
                    game->player_jeep.active = false;
                }
            }
        }
    }
    
    // Check game over
    if (!game->player_helicopter.active && !game->player_jeep.active) {
        game->state = GAME_OVER;
    }
}

void render_game(Game* game) {
    // Clear renderer
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
    SDL_RenderClear(game->renderer);
    
    // Render scrolling background
    SDL_Rect bg_rect1 = {0, game->bg_scroll_offset, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_Rect bg_rect2 = {0, game->bg_scroll_offset - SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(game->renderer, game->background, &bg_rect1, NULL);
    SDL_RenderCopy(game->renderer, game->background, &bg_rect2, NULL);
    
    // Render players
    if (game->player_helicopter.active) {
        SDL_Rect helicopter_rect = {
            (int)game->player_helicopter.x,
            (int)game->player_helicopter.y,
            game->player_helicopter.width,
            game->player_helicopter.height
        };
        SDL_RenderCopy(game->renderer, game->helicopter, NULL, &helicopter_rect);
    }
    
    if (game->player_jeep.active) {
        SDL_Rect jeep_rect = {
            (int)game->player_jeep.x,
            (int)game->player_jeep.y,
            game->player_jeep.width,
            game->player_jeep.height
        };
        SDL_RenderCopy(game->renderer, game->jeep, NULL, &jeep_rect);
    }
    
    // Render bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game->bullets[i].active) {
            SDL_Rect bullet_rect = {
                (int)game->bullets[i].x,
                (int)game->bullets[i].y,
                game->bullets[i].width,
                game->bullets[i].height
            };
            SDL_RenderCopy(game->renderer, game->bullet, NULL, &bullet_rect);
        }
    }
    
    // Render enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].active) {
            SDL_Rect enemy_rect = {
                (int)game->enemies[i].x,
                (int)game->enemies[i].y,
                game->enemies[i].width,
                game->enemies[i].height
            };
            SDL_RenderCopy(game->renderer, game->enemy, NULL, &enemy_rect);
        }
    }
    
    // Render game over text if needed
    if (game->state == GAME_OVER) {
        // In a real game, you would render text here using SDL_ttf
        // For this example, we'll just draw a rectangle
        SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
        SDL_Rect game_over_rect = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 3, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 3};
        SDL_RenderFillRect(game->renderer, &game_over_rect);
    }
    
    // Present renderer
    SDL_RenderPresent(game->renderer);
}

void spawn_enemy(Game* game) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!game->enemies[i].active) {
            game->enemies[i].active = true;
            game->enemies[i].x = SCREEN_WIDTH;
            game->enemies[i].y = rand() % (SCREEN_HEIGHT - game->enemies[i].height);
            game->enemies[i].health = 1;
            break;
        }
    }
}

void spawn_bullet(Game* game, Entity* player) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!game->bullets[i].active) {
            game->bullets[i].active = true;
            game->bullets[i].x = player->x + player->width;
            game->bullets[i].y = player->y + (player->height / 2);
            break;
        }
    }
}

bool check_collision(Entity* a, Entity* b) {
    return (a->x < b->x + b->width &&
            a->x + a->width > b->x &&
            a->y < b->y + b->height &&
            a->y + a->height > b->y);
}
