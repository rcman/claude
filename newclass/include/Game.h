#ifndef GAME_H
#define GAME_H

#include "Common.h"
#include "Camera.h"
#include "Player.h"
#include "Zombie.h"
#include "Building.h"
#include "Item.h"
#include "TileMap.h"
#include "UIManager.h"

class Game {
private:
    bool running;
    SDL_Window* window;
    SDL_Renderer* renderer;
    GameState gameState;
    TimeOfDay timeOfDay;
    Uint32 gameTime;
    Uint32 lastFrameTime;
    Uint32 lastTimeUpdate;
    Camera camera;

    SDL_Texture* playerTexture;
    SDL_Texture* zombieTextures[4];
    SDL_Texture* buildingTextures[3];
    SDL_Texture* itemTextures[8];
    SDL_Texture* tilesetTexture;
    SDL_Texture* uiTextures[2];
    TTF_Font* font;

    std::unique_ptr<Player> player;
    std::vector<std::unique_ptr<Zombie>> zombies;
    std::vector<std::unique_ptr<Building>> buildings;
    std::vector<std::unique_ptr<Item>> items;
    std::unique_ptr<TileMap> tileMap;
    std::unique_ptr<UIManager> uiManager;

    std::mt19937 rng;
    Uint32 lastZombieSpawn;
    Uint32 zombieSpawnInterval;

    SDL_Texture* createColorTexture(int width, int height, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void setupGame();
    void createBuildings();
    void createItemInBuilding(Building& building);
    void spawnZombie();
    void spawnItem();
    void handleMainMenuKeys(SDL_Keycode key);
    void handleGameplayKeys(SDL_Keycode key);
    void handleInventoryKeys(SDL_Keycode key);
    void handleCraftingKeys(SDL_Keycode key);
    void handlePauseKeys(SDL_Keycode key);
    void handleGameOverKeys(SDL_Keycode key);
    void handleInteraction();
    void transferItems();
    void craftWeaponUpgrade();
    void craftArmorUpgrade();
    void craftHealthPack();
    void craftAmmo();

public:
    Game();
    ~Game();
    bool init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
    bool loadMedia();
    void handleEvents();
    void handleKeyDown(SDL_Keycode key);
    void update();
    void render();
    void clean();
    bool isRunning() const;
    void capFrameRate(Uint32 frameStart);
};

#endif // GAME_H
