#include "UIManager.h"

UIManager::UIManager(SDL_Renderer* ren, TTF_Font* f, SDL_Texture* hpBar, SDL_Texture* invTex, GameState& state, Player& p, TimeOfDay& time) :
    renderer(ren), font(f), hpBarTexture(hpBar), inventoryTexture(invTex), gameState(state), player(p), timeOfDay(time) {}

void UIManager::renderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font) {
        std::cout << "Warning: UIManager has nullptr font, skipping text render." << std::endl;
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    SDL_Rect destRect = { x, y, w, h };

    SDL_RenderCopy(renderer, texture, nullptr, &destRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void UIManager::renderHealthBar() {
    int health = player.getHealth();
    int maxHealth = player.getMaxHealth();

    SDL_Rect bgRect = { 20, 20, 200, 30 };
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_Rect fillRect = { 20, 20, static_cast<int>(200.0f * health / maxHealth), 30 };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &fillRect);

    SDL_Color white = { 255, 255, 255, 255 };
    renderText("HP: " + std::to_string(health) + "/" + std::to_string(maxHealth), 25, 25, white);
}

void UIManager::renderInventoryPreview() {
    SDL_Color white = { 255, 255, 255, 255 };
    Inventory& inv = player.getInventory();
    renderText("Wood: " + std::to_string(inv.getItemCount(RESOURCE_WOOD)), 20, 60, white);
    renderText("Metal: " + std::to_string(inv.getItemCount(RESOURCE_METAL)), 20, 85, white);
    renderText("Food: " + std::to_string(inv.getItemCount(RESOURCE_FOOD)), 20, 110, white);
    renderText("Cloth: " + std::to_string(inv.getItemCount(RESOURCE_CLOTH)), 20, 135, white);
    renderText("Ammo: " + std::to_string(inv.getItemCount(AMMO)), 20, 160, white);
}

void UIManager::renderTimeOfDay() {
    SDL_Color color;
    std::string timeText;

    if (timeOfDay == DAY) {
        color = { 255, 255, 0, 255 };
        timeText = "Day";
    } else {
        color = { 100, 100, 255, 255 };
        timeText = "Night";
    }

    renderText(timeText, SCREEN_WIDTH - 100, 20, color);
}

void UIManager::renderGameplay() {
    renderHealthBar();
    renderInventoryPreview();
    renderTimeOfDay();

    if (player.getHomeBase()) {
        SDL_Color green = { 0, 255, 0, 255 };
        renderText("Home Base Set", SCREEN_WIDTH - 200, 50, green);
    }

    if (player.getIsInside()) {
        SDL_Color cyan = { 0, 255, 255, 255 };
        renderText("Inside Building", SCREEN_WIDTH - 200, 80, cyan);
    }
}

void UIManager::renderInventoryScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_Color white = { 255, 255, 255, 255 };
    renderText("Inventory", SCREEN_WIDTH / 2 - 100, 50, white);

    Inventory& inv = player.getInventory();
    int y = 120;

    renderText("Resources:", 300, y, white); y += 40;
    renderText("Wood: " + std::to_string(inv.getItemCount(RESOURCE_WOOD)), 320, y, white); y += 30;
    renderText("Metal: " + std::to_string(inv.getItemCount(RESOURCE_METAL)), 320, y, white); y += 30;
    renderText("Food: " + std::to_string(inv.getItemCount(RESOURCE_FOOD)), 320, y, white); y += 30;
    renderText("Cloth: " + std::to_string(inv.getItemCount(RESOURCE_CLOTH)), 320, y, white); y += 50;

    if (player.getHomeBase() && player.getIsInside()) {
        Inventory& storage = player.getHomeBase()->getStorage();
        y = 160;
        renderText("Ammo: " + std::to_string(storage.getItemCount(AMMO)), SCREEN_WIDTH - 480, y, white);
    }
}

void UIManager::renderCraftingScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_Color white = { 255, 255, 255, 255 };
    renderText("Crafting", SCREEN_WIDTH / 2 - 80, 50, white);

    Inventory& inv = player.getInventory();
    int y = 120;

    renderText("Resources:", 200, y, white); y += 40;
    renderText("Wood: " + std::to_string(inv.getItemCount(RESOURCE_WOOD)), 220, y, white); y += 30;
    renderText("Metal: " + std::to_string(inv.getItemCount(RESOURCE_METAL)), 220, y, white); y += 30;
    renderText("Food: " + std::to_string(inv.getItemCount(RESOURCE_FOOD)), 220, y, white); y += 30;
    renderText("Cloth: " + std::to_string(inv.getItemCount(RESOURCE_CLOTH)), 220, y, white); y += 50;

    y = 120;
    renderText("Crafting Options:", SCREEN_WIDTH - 600, y, white); y += 40;
    renderText("1. Upgrade Weapon (5 Metal, 3 Wood) [Press 1]", SCREEN_WIDTH - 580, y, white); y += 30;
    renderText("2. Upgrade Armor (8 Cloth, 2 Metal) [Press 2]", SCREEN_WIDTH - 580, y, white); y += 30;
    renderText("3. Craft Health Pack (5 Food, 2 Cloth) [Press 3]", SCREEN_WIDTH - 580, y, white); y += 30;
    renderText("4. Craft Ammo (3 Metal, 2 Wood) [Press 4]", SCREEN_WIDTH - 580, y, white); y += 30;

    renderText("Press [C] to close crafting menu", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT - 50, white);
}

void UIManager::renderPauseScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);
}

void UIManager::renderGameOverScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);
}

void UIManager::renderMainMenu() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
    SDL_Rect bgRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_Color title = { 255, 200, 0, 255 };
    SDL_Color normal = { 200, 200, 200, 255 };

    renderText("ZOMBIE SURVIVAL", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 4, title);
    renderText("Press [ENTER] to start game", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2, normal);
    renderText("Press [ESC] to quit", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50, normal);

    int y = SCREEN_HEIGHT - 200;
    renderText("Controls:", 100, y, normal); y += 30;
    renderText("WASD - Move", 120, y, normal); y += 25;
    renderText("E - Interact with buildings/items", 120, y, normal); y += 25;
    renderText("I - Inventory", 120, y, normal); y += 25;
    renderText("C - Crafting", 120, y, normal); y += 25;
    renderText("H - Set current building as home base", 120, y, normal);
}

void UIManager::render() {
    switch (gameState) {
        case MAIN_MENU:
            renderMainMenu();
            break;
        case GAMEPLAY:
            renderGameplay();
            break;
        case INVENTORY:
            renderGameplay();
            renderInventoryScreen();
            break;
        case CRAFTING:
            renderGameplay();
            renderCraftingScreen();
            break;
        case PAUSE:
            renderGameplay();
            renderPauseScreen();
            break;
        case GAME_OVER:
            renderGameOverScreen();
            break;
    }
}
