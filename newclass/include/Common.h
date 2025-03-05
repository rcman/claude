#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <random>
#include <memory>

// Constants
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int MAP_WIDTH = 2000;
const int MAP_HEIGHT = 2000;
const int TILE_SIZE = 32;
const int PLAYER_SPEED = 5;
const int MAX_FPS = 60;
const int FRAME_DELAY = 1000 / MAX_FPS;
const int DAY_NIGHT_CYCLE_DURATION = 600000; // 10 minutes in milliseconds
const float DAY_RATIO = 0.7f; // 70% day, 30% night

// Enums
enum GameState {
    MAIN_MENU,
    GAMEPLAY,
    INVENTORY,
    CRAFTING,
    GAME_OVER,
    PAUSE
};

enum ItemType {
    WEAPON,
    ARMOR,
    HEALTH,
    RESOURCE_WOOD,
    RESOURCE_METAL,
    RESOURCE_FOOD,
    RESOURCE_CLOTH,
    AMMO
};

enum ZombieType {
    NORMAL,
    RUNNER,
    TANK,
    EXPLODER
};

enum TimeOfDay {
    DAY,
    NIGHT
};

enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
};

// Utility function
float distance(float x1, float y1, float x2, float y2);

#endif // COMMON_H
