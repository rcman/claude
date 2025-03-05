#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "Common.h"
#include "Player.h"

class UIManager {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* hpBarTexture;
    SDL_Texture* inventoryTexture;
    GameState& gameState;
    Player& player;
    TimeOfDay& timeOfDay;

    void renderHealthBar();
    void renderInventoryPreview();
    void renderTimeOfDay();
    void renderGameplay();
    void renderInventoryScreen();
    void renderCraftingScreen();
    void renderPauseScreen();
    void renderGameOverScreen();
    void renderMainMenu();

public:
    UIManager(SDL_Renderer* ren, TTF_Font* f, SDL_Texture* hpBar, SDL_Texture* invTex, GameState& state, Player& p, TimeOfDay& time);
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void render();
};

#endif // UIMANAGER_H
