#include "Game.h"
#include <stdio.h>
#include <algorithm>

Game::Game() :
    running(true), window(nullptr), renderer(nullptr), gameState(GAMEPLAY),
    timeOfDay(DAY), gameTime(0), lastFrameTime(0), lastTimeUpdate(0),
    camera(MAP_WIDTH, MAP_HEIGHT), lastZombieSpawn(0), zombieSpawnInterval(5000),
    playerTexture(nullptr), tilesetTexture(nullptr), font(nullptr) {
    std::random_device rd;
    rng = std::mt19937(rd());
    for (int i = 0; i < 4; ++i) zombieTextures[i] = nullptr;
    for (int i = 0; i < 3; ++i) buildingTextures[i] = nullptr;
    for (int i = 0; i < 8; ++i) itemTextures[i] = nullptr;
    for (int i = 0; i < 2; ++i) uiTextures[i] = nullptr;
}

Game::~Game() {
    clean();
}

bool Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    int flags = 0;
    if (fullscreen) flags = SDL_WINDOW_FULLSCREEN;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "SDL initialized successfully" << std::endl;

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image initialization failed: " << IMG_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() != 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "Game initialized - Window: " << width << "x" << height
              << ", Fullscreen: " << (fullscreen ? "Yes" : "No") << std::endl;

    running = true;
    return loadMedia();
}

bool Game::loadMedia() {
    playerTexture = createColorTexture(TILE_SIZE, TILE_SIZE, 0, 0, 255, 255);
    if (!playerTexture) return false;

    zombieTextures[NORMAL] = createColorTexture(TILE_SIZE, TILE_SIZE, 0, 255, 0, 255);
    if (!zombieTextures[NORMAL]) return false;
    zombieTextures[RUNNER] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 255, 0, 255);
    if (!zombieTextures[RUNNER]) return false;
    zombieTextures[TANK] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 128, 0, 255);
    if (!zombieTextures[TANK]) return false;
    zombieTextures[EXPLODER] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 0, 0, 255);
    if (!zombieTextures[EXPLODER]) return false;

    buildingTextures[0] = createColorTexture(TILE_SIZE * 4, TILE_SIZE * 3, 128, 128, 128, 255);
    if (!buildingTextures[0]) return false;
    buildingTextures[1] = createColorTexture(TILE_SIZE * 6, TILE_SIZE * 4, 150, 150, 150, 255);
    if (!buildingTextures[1]) return false;
    buildingTextures[2] = createColorTexture(TILE_SIZE * 8, TILE_SIZE * 6, 170, 170, 170, 255);
    if (!buildingTextures[2]) return false;

    itemTextures[WEAPON] = createColorTexture(TILE_SIZE, TILE_SIZE, 192, 192, 0, 255);
    if (!itemTextures[WEAPON]) return false;
    itemTextures[ARMOR] = createColorTexture(TILE_SIZE, TILE_SIZE, 192, 0, 192, 255);
    if (!itemTextures[ARMOR]) return false;
    itemTextures[HEALTH] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 0, 0, 255);
    if (!itemTextures[HEALTH]) return false;
    itemTextures[RESOURCE_WOOD] = createColorTexture(TILE_SIZE, TILE_SIZE, 139, 69, 19, 255);
    if (!itemTextures[RESOURCE_WOOD]) return false;
    itemTextures[RESOURCE_METAL] = createColorTexture(TILE_SIZE, TILE_SIZE, 169, 169, 169, 255);
    if (!itemTextures[RESOURCE_METAL]) return false;
    itemTextures[RESOURCE_FOOD] = createColorTexture(TILE_SIZE, TILE_SIZE, 0, 128, 0, 255);
    if (!itemTextures[RESOURCE_FOOD]) return false;
    itemTextures[RESOURCE_CLOTH] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 255, 255, 255);
    if (!itemTextures[RESOURCE_CLOTH]) return false;
    itemTextures[AMMO] = createColorTexture(TILE_SIZE, TILE_SIZE, 255, 215, 0, 255);
    if (!itemTextures[AMMO]) return false;

    tilesetTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TILE_SIZE * 3, TILE_SIZE);
    if (!tilesetTexture) {
        std::cerr << "Tileset texture creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderTarget(renderer, tilesetTexture);
    SDL_Rect rect = { 0, 0, TILE_SIZE, TILE_SIZE };
    SDL_SetRenderDrawColor(renderer, 100, 150, 100, 255);
    SDL_RenderFillRect(renderer, &rect);

    rect.x = TILE_SIZE;
    SDL_SetRenderDrawColor(renderer, 120, 170, 120, 255);
    SDL_RenderFillRect(renderer, &rect);

    rect.x = TILE_SIZE * 2;
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderTarget(renderer, nullptr);

    uiTextures[0] = createColorTexture(200, 30, 255, 0, 0, 255);
    if (!uiTextures[0]) return false;
    uiTextures[1] = createColorTexture(400, 300, 50, 50, 50, 200);
    if (!uiTextures[1]) return false;

    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cerr << "Font loading failed: " << TTF_GetError() << std::endl;
        return false;
    }

    setupGame();
    gameState = GAMEPLAY;
    return true;
}

SDL_Texture* Game::createColorTexture(int width, int height, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (!texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_Rect rect = { 0, 0, width, height };
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderTarget(renderer, nullptr);

    return texture;
}

void Game::setupGame() {
    player = std::make_unique<Player>(Vector2D(MAP_WIDTH / 2, MAP_HEIGHT / 2), playerTexture);
    std::cout << "New game setup - Player spawned at ("
              << MAP_WIDTH / 2 << ", " << MAP_HEIGHT / 2 << ")" << std::endl;

    tileMap = std::make_unique<TileMap>(tilesetTexture, MAP_WIDTH, MAP_HEIGHT, TILE_SIZE);

    createBuildings();

    for (int i = 0; i < 10; ++i) {
        spawnZombie();
    }

    for (int i = 0; i < 20; ++i) {
        spawnItem();
    }

    uiManager = std::make_unique<UIManager>(renderer, font, uiTextures[0], uiTextures[1], gameState, *player, timeOfDay);

    gameTime = 0;
    lastFrameTime = SDL_GetTicks();
    lastTimeUpdate = lastFrameTime;
    lastZombieSpawn = lastFrameTime;

    gameState = GAMEPLAY;
    timeOfDay = DAY;

    std::cout << "Game setup complete - Zombies: " << zombies.size()
              << ", Items: " << items.size() << ", Buildings: "
              << buildings.size() << std::endl;
}

void Game::createBuildings() {
    std::uniform_int_distribution<int> xPosDist(TILE_SIZE * 5, MAP_WIDTH - TILE_SIZE * 10);
    std::uniform_int_distribution<int> yPosDist(TILE_SIZE * 5, MAP_HEIGHT - TILE_SIZE * 10);
    std::uniform_int_distribution<int> buildingTypeDist(0, 2);

    for (int i = 0; i < 15; ++i) {
        int type = buildingTypeDist(rng);
        int width, height;

        switch (type) {
            case 0: width = TILE_SIZE * 4; height = TILE_SIZE * 3; break;
            case 1: width = TILE_SIZE * 6; height = TILE_SIZE * 4; break;
            case 2: width = TILE_SIZE * 8; height = TILE_SIZE * 6; break;
        }

        bool validPosition = false;
        Vector2D pos;

        int attempts = 0;
        while (!validPosition && attempts < 20) {
            validPosition = true;
            pos.x = xPosDist(rng);
            pos.y = yPosDist(rng);

            Rectangle newRect(pos.x, pos.y, width, height);

            for (const auto& building : buildings) {
                Rectangle existingRect = building->getCollider();
                existingRect.x -= TILE_SIZE * 2;
                existingRect.y -= TILE_SIZE * 2;
                existingRect.w += TILE_SIZE * 4;
                existingRect.h += TILE_SIZE * 4;

                if (existingRect.intersects(newRect)) {
                    validPosition = false;
                    break;
                }
            }

            attempts++;
        }

        if (validPosition) {
            auto building = std::make_unique<Building>(pos, buildingTextures[type], width, height);

            std::uniform_int_distribution<int> itemCountDist(1, 5);
            int itemCount = itemCountDist(rng);

            for (int j = 0; j < itemCount; ++j) {
                createItemInBuilding(*building);
            }

            buildings.push_back(std::move(building));
        }
    }
}

void Game::createItemInBuilding(Building& building) {
    std::uniform_int_distribution<int> itemTypeDist(0, 7);
    std::uniform_int_distribution<int> valueDist(1, 5);

    ItemType type = static_cast<ItemType>(itemTypeDist(rng));
    int value = valueDist(rng);
    std::string name;

    switch (type) {
        case WEAPON: name = "Weapon Upgrade"; break;
        case ARMOR: name = "Armor Piece"; break;
        case HEALTH: name = "Health Pack"; break;
        case RESOURCE_WOOD: name = "Wood"; break;
        case RESOURCE_METAL: name = "Metal"; break;
        case RESOURCE_FOOD: name = "Food"; break;
        case RESOURCE_CLOTH: name = "Cloth"; break;
        case AMMO: name = "Ammo"; break;
    }

    Item* item = new Item(Vector2D(0, 0), itemTextures[type], type, value, name);
    building.addItem(item);
}

void Game::spawnZombie() {
    std::uniform_real_distribution<float> xPosDist(0, MAP_WIDTH - TILE_SIZE);
    std::uniform_real_distribution<float> yPosDist(0, MAP_HEIGHT - TILE_SIZE);
    std::uniform_int_distribution<int> typeDist(0, 3);

    Vector2D playerPos = player->getPosition();
    Vector2D pos;
    bool validPosition = false;

    while (!validPosition) {
        pos.x = xPosDist(rng);
        pos.y = yPosDist(rng);

        float dist = distance(pos.x, pos.y, playerPos.x, playerPos.y);
        if (dist > 300) {
            validPosition = true;

            for (const auto& building : buildings) {
                if (building->isInside(pos)) {
                    validPosition = false;
                    break;
                }
            }
        }
    }

    ZombieType type;

    if (timeOfDay == NIGHT) {
        std::uniform_int_distribution<int> nightTypeDist(0, 10);
        int roll = nightTypeDist(rng);

        if (roll < 4) type = NORMAL;
        else if (roll < 7) type = RUNNER;
        else if (roll < 9) type = TANK;
        else type = EXPLODER;
    } else {
        type = static_cast<ZombieType>(typeDist(rng));
    }

    zombies.push_back(std::make_unique<Zombie>(pos, zombieTextures[type], type));
    std::cout << "Zombie spawned - Type: " << type << " at ("
              << pos.x << ", " << pos.y << "), Total zombies: "
              << zombies.size() << std::endl;
}

void Game::spawnItem() {
    std::uniform_real_distribution<float> xPosDist(TILE_SIZE, MAP_WIDTH - TILE_SIZE * 2);
    std::uniform_real_distribution<float> yPosDist(TILE_SIZE, MAP_HEIGHT - TILE_SIZE * 2);
    std::uniform_int_distribution<int> itemTypeDist(0, 7);
    std::uniform_int_distribution<int> valueDist(1, 5);

    ItemType type = static_cast<ItemType>(itemTypeDist(rng));

    if (timeOfDay == NIGHT) {
        std::uniform_int_distribution<int> nightTypeDist(0, 10);
        int roll = nightTypeDist(rng);

        if (roll < 3) {
            type = static_cast<ItemType>(itemTypeDist(rng));
        } else if (roll < 6) {
            std::uniform_int_distribution<int> valuableTypeDist(0, 2);
            int valuableType = valuableTypeDist(rng);
            if (valuableType == 0) type = WEAPON;
            else if (valuableType == 1) type = ARMOR;
            else type = AMMO;
        } else {
            std::uniform_int_distribution<int> veryValuableTypeDist(0, 2);
            int veryValuableType = veryValuableTypeDist(rng);
            if (veryValuableType == 0) type = WEAPON;
            else if (veryValuableType == 1) type = ARMOR;
            else type = HEALTH;
        }
    }

    int value = valueDist(rng);
    if (timeOfDay == NIGHT) value = value * 2;

    std::string name;
    switch (type) {
        case WEAPON: name = "Weapon Upgrade"; break;
        case ARMOR: name = "Armor Piece"; break;
        case HEALTH: name = "Health Pack"; break;
        case RESOURCE_WOOD: name = "Wood"; break;
        case RESOURCE_METAL: name = "Metal"; break;
        case RESOURCE_FOOD: name = "Food"; break;
        case RESOURCE_CLOTH: name = "Cloth"; break;
        case AMMO: name = "Ammo"; break;
    }

    Vector2D pos;
    bool validPosition = false;

    while (!validPosition) {
        pos.x = xPosDist(rng);
        pos.y = yPosDist(rng);

        validPosition = true;

        if (tileMap->checkCollision(Rectangle(pos.x, pos.y, TILE_SIZE, TILE_SIZE))) {
            validPosition = false;
            continue;
        }

        for (const auto& building : buildings) {
            if (building->isInside(pos)) {
                validPosition = false;
                break;
            }
        }
    }

    items.push_back(std::make_unique<Item>(pos, itemTextures[type], type, value, name));
}

void Game::handleEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                handleKeyDown(event.key.keysym.sym);
                break;

            default:
                break;
        }
    }

    if (gameState == GAMEPLAY) {
        const Uint8* keystates = SDL_GetKeyboardState(nullptr);
        player->handleInput(keystates);
    }
}

void Game::handleKeyDown(SDL_Keycode key) {
    switch (gameState) {
        case MAIN_MENU:
            handleMainMenuKeys(key);
            break;
        case GAMEPLAY:
            handleGameplayKeys(key);
            break;
        case INVENTORY:
            handleInventoryKeys(key);
            break;
        case CRAFTING:
            handleCraftingKeys(key);
            break;
        case PAUSE:
            handlePauseKeys(key);
            break;
        case GAME_OVER:
            handleGameOverKeys(key);
            break;
    }
}

void Game::handleMainMenuKeys(SDL_Keycode key) {
    switch (key) {
        case SDLK_RETURN:
            setupGame();
            gameState = GAMEPLAY;
            break;
        case SDLK_ESCAPE:
            running = false;
            break;
    }
}

void Game::handleGameplayKeys(SDL_Keycode key) {
    switch (key) {
        case SDLK_ESCAPE:
            gameState = PAUSE;
            break;
        case SDLK_i:
            gameState = INVENTORY;
            break;
        case SDLK_c:
            gameState = CRAFTING;
            break;
        case SDLK_e:
            handleInteraction();
            break;
        case SDLK_h:
            if (player->getIsInside()) {
                for (auto& building : buildings) {
                    if (building->isInside(player->getPosition())) {
                        player->setHomeBase(building.get());
                        building->setHomeBase(true);
                        break;
                    }
                }
            }
            break;
    }
}

void Game::handleInventoryKeys(SDL_Keycode key) {
    switch (key) {
        case SDLK_i:
        case SDLK_ESCAPE:
            gameState = GAMEPLAY;
            break;
        case SDLK_t:
            if (player->getHomeBase() && player->getIsInside()) {
                transferItems();
            }
            break;
    }
}

void Game::handleCraftingKeys(SDL_Keycode key) {
    switch (key) {
        case SDLK_c:
        case SDLK_ESCAPE:
            gameState = GAMEPLAY;
            break;
        case SDLK_1:
            craftWeaponUpgrade();
            break;
        case SDLK_2:
            craftArmorUpgrade();
            break;
        case SDLK_3:
            craftHealthPack();
            break;
        case SDLK_4:
            craftAmmo();
            break;
    }
}

void Game::handlePauseKeys(SDL_Keycode key) {
    switch (key) {
        case SDLK_p:
            gameState = GAMEPLAY;
            break;
        case SDLK_ESCAPE:
            running = false;
            break;
    }
}

void Game::handleGameOverKeys(SDL_Keycode key) {
    switch (key) {
        case SDLK_r:
            setupGame();
            break;
        case SDLK_ESCAPE:
            running = false;
            break;
    }
}

void Game::handleInteraction() {
    bool enteredBuilding = false;

    for (auto& building : buildings) {
        if (building->isAtEntrance(player->getPosition())) {
            player->setIsInside(!player->getIsInside());
            enteredBuilding = true;
            std::cout << "Player " << (player->getIsInside() ? "entered" : "exited")
                      << " building at (" << building->getPosition().x
                      << ", " << building->getPosition().y << ")" << std::endl;
            break;
        }
    }

    if (!enteredBuilding && player->getIsInside()) {
        bool stillInside = false;
        for (auto& building : buildings) {
            if (building->isInside(player->getPosition())) {
                stillInside = true;
                break;
            }
        }

        if (!stillInside) {
            player->setIsInside(false);
        }
    }

    if (!player->getIsInside()) {
        for (auto it = items.begin(); it != items.end(); ) {
            if ((*it)->checkCollision(*player)) {
                bool added = player->getInventory().addItem(**it);
                std::cout << "Item pickup attempted: " << (*it)->getName()
                          << " - Success: " << (added ? "Yes" : "No") << std::endl;
                if (added) {
                    it = items.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    } else {
        for (auto& building : buildings) {
            if (building->isInside(player->getPosition())) {
                auto& buildingItems = building->getItems();
                if (!buildingItems.empty()) {
                    Item* item = buildingItems.front();
                    bool added = player->getInventory().addItem(*item);
                    if (added) {
                        buildingItems.erase(buildingItems.begin());
                        delete item;
                    }
                }
                break;
            }
        }
    }
}

void Game::transferItems() {
    Building* homeBase = player->getHomeBase();
    if (!homeBase || !player->getIsInside()) return;

    Inventory& playerInventory = player->getInventory();
    Inventory& storage = homeBase->getStorage();

    playerInventory.transferTo(storage, RESOURCE_WOOD, 1000);
    playerInventory.transferTo(storage, RESOURCE_METAL, 1000);
    playerInventory.transferTo(storage, RESOURCE_FOOD, 1000);
    playerInventory.transferTo(storage, RESOURCE_CLOTH, 1000);

    storage.transferTo(playerInventory, WEAPON, 1000);
    storage.transferTo(playerInventory, ARMOR, 1000);
    storage.transferTo(playerInventory, HEALTH, 1000);
    storage.transferTo(playerInventory, AMMO, 1000);
}

void Game::craftWeaponUpgrade() {
    Inventory& inv = player->getInventory();
    if (inv.getItemCount(RESOURCE_METAL) >= 5 && inv.getItemCount(RESOURCE_WOOD) >= 3) {
        inv.useItem(RESOURCE_METAL, 5);
        inv.useItem(RESOURCE_WOOD, 3);
        player->upgradeWeapon(10);
    }
}

void Game::craftArmorUpgrade() {
    Inventory& inv = player->getInventory();
    if (inv.getItemCount(RESOURCE_CLOTH) >= 8 && inv.getItemCount(RESOURCE_METAL) >= 2) {
        inv.useItem(RESOURCE_CLOTH, 8);
        inv.useItem(RESOURCE_METAL, 2);
        player->upgradeArmor(10);
    }
}

void Game::craftHealthPack() {
    Inventory& inv = player->getInventory();
    if (inv.getItemCount(RESOURCE_FOOD) >= 5 && inv.getItemCount(RESOURCE_CLOTH) >= 2) {
        inv.useItem(RESOURCE_FOOD, 5);
        inv.useItem(RESOURCE_CLOTH, 2);
        inv.addItem(Item(Vector2D(0, 0), itemTextures[HEALTH], HEALTH, 1, "Health Pack"));
    }
}

void Game::craftAmmo() {
    Inventory& inv = player->getInventory();
    if (inv.getItemCount(RESOURCE_METAL) >= 3 && inv.getItemCount(RESOURCE_WOOD) >= 2) {
        inv.useItem(RESOURCE_METAL, 3);
        inv.useItem(RESOURCE_WOOD, 2);
        inv.addItem(Item(Vector2D(0, 0), itemTextures[AMMO], AMMO, 10, "Ammo"));
    }
}

void Game::update() {
    Uint32 currentTime = SDL_GetTicks();
    Uint32 deltaTime = currentTime - lastFrameTime;

    std::cout << "Game update - Time: " << gameTime
              << "ms, Delta: " << deltaTime << "ms" << std::endl;

    lastFrameTime = currentTime;

    if (gameState != GAMEPLAY) {
        printf("gamestate=%d, gameplay=%d\n", gameState, GAMEPLAY);
        return;
    }

    printf("about to run gametime += deltatime");
    gameTime += deltaTime;

    if (currentTime - lastTimeUpdate > 1000) {
        Uint32 cycleTime = gameTime % DAY_NIGHT_CYCLE_DURATION;
        TimeOfDay newTimeOfDay = (cycleTime < DAY_NIGHT_CYCLE_DURATION * DAY_RATIO) ? DAY : NIGHT;

        if (newTimeOfDay != timeOfDay) {
            timeOfDay = newTimeOfDay;

            if (timeOfDay == NIGHT) {
                int nightSpawnCount = 10 + rand() % 10;
                for (int i = 0; i < nightSpawnCount; ++i) {
                    spawnZombie();
                }

                int nightItemCount = 5 + rand() % 5;
                for (int i = 0; i < nightItemCount; ++i) {
                    spawnItem();
                }
            }
        }

        lastTimeUpdate = currentTime;
    }

    printf("before player update");
    player->update();
    printf("After player update");

    bool inBuilding = false;

    for (auto& building : buildings) {
        if (player->getIsInside()) {
            if (building->isInside(player->getPosition())) {
                inBuilding = true;
            }
        } else {
            if (player->checkCollision(*building)) {
                Vector2D pushDirection = (player->getPosition() - building->getPosition()).normalize();
                player->setPosition(player->getPosition() + pushDirection * 5.0f);
            }
        }
    }

    if (player->getIsInside() && !inBuilding) {
        player->setIsInside(false);
    }

    for (auto& zombie : zombies) {
        if (zombie->isActive()) {
            bool zombieInBuilding = false;
            for (auto& building : buildings) {
                if (building->isInside(zombie->getPosition())) {
                    zombieInBuilding = true;
                    break;
                }
            }

            if (!zombieInBuilding) {
                zombie->update(*player);

                for (auto& building : buildings) {
                    if (zombie->checkCollision(*building)) {
                        Vector2D pushDirection = (zombie->getPosition() - building->getPosition()).normalize();
                        zombie->setPosition(zombie->getPosition() + pushDirection * 3.0f);
                    }
                }
            }
        }
    }

    for (auto& zombie : zombies) {
        if (zombie->isActive() && !player->getIsInside()) {
            if (zombie->checkCollision(*player)) {
                if (player->getInventory().getItemCount(AMMO) > 0) {
                    zombie->takeDamage(player->getWeaponPower() * 2);
                    player->getInventory().useItem(AMMO, 1);
                } else {
                    zombie->takeDamage(player->getWeaponPower());
                }
            }
        }
    }

    zombies.erase(
        std::remove_if(zombies.begin(), zombies.end(),
            [](const std::unique_ptr<Zombie>& z) { return !z->isActive(); }),
        zombies.end()
    );

    if (currentTime - lastZombieSpawn > zombieSpawnInterval) {
        int spawnCount = (timeOfDay == DAY) ? 1 : 3;
        for (int i = 0; i < spawnCount; ++i) {
            spawnZombie();
        }
        lastZombieSpawn = currentTime;
    }

    if (rand() % 100 < 1) {
        spawnItem();
    }

    if (player->getHealth() < player->getMaxHealth() / 2 && player->getInventory().getItemCount(HEALTH) > 0) {
        player->getInventory().useItem(HEALTH, 1);
        player->heal(25);
    }

    if (player->getHealth() <= 0) {
        gameState = GAME_OVER;
        std::cout << "Game Over - Player health depleted" << std::endl;
    }

    camera.update(player->getPosition());
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (gameState == MAIN_MENU) {
        uiManager->render();
        SDL_RenderPresent(renderer);
        return;
    }

    SDL_Rect viewport = camera.getViewport();
    tileMap->render(renderer, camera);

    for (auto& building : buildings) {
        if (camera.isVisible(building->getCollider())) {
            building->render(renderer, camera);

            if (building->isHomeBase()) {
                Vector2D screenPos = camera.worldToScreen(building->getPosition());
                SDL_Rect outlineRect = {
                    static_cast<int>(screenPos.x) - 2,
                    static_cast<int>(screenPos.y) - 2,
                    static_cast<int>(building->getCollider().w) + 4,
                    static_cast<int>(building->getCollider().h) + 4
                };

                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderDrawRect(renderer, &outlineRect);
            }
        }
    }

    for (auto& item : items) {
        if (camera.isVisible(item->getCollider())) {
            item->render(renderer, camera);
        }
    }

    for (auto& zombie : zombies) {
        if (zombie->isActive() && camera.isVisible(zombie->getCollider())) {
            zombie->render(renderer, camera);
        }
    }

    if (!player->getIsInside()) {
        player->render(renderer, camera);
    }

    // uiManager->render();
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    if (playerTexture) SDL_DestroyTexture(playerTexture);
    for (int i = 0; i < 4; ++i) if (zombieTextures[i]) SDL_DestroyTexture(zombieTextures[i]);
    for (int i = 0; i < 3; ++i) if (buildingTextures[i]) SDL_DestroyTexture(buildingTextures[i]);
    for (int i = 0; i < 8; ++i) if (itemTextures[i]) SDL_DestroyTexture(itemTextures[i]);
    if (tilesetTexture) SDL_DestroyTexture(tilesetTexture);
    for (int i = 0; i < 2; ++i) if (uiTextures[i]) SDL_DestroyTexture(uiTextures[i]);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

bool Game::isRunning() const {
    return running;
}

void Game::capFrameRate(Uint32 frameStart) {
    Uint32 frameTime = SDL_GetTicks() - frameStart;
    if (frameTime < FRAME_DELAY) {
        SDL_Delay(FRAME_DELAY - frameTime);
    }
}
