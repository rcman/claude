#include "TileMap.h"

TileMap::TileMap(SDL_Texture* tiles, int width, int height, int tileSize) :
    tileset(tiles), mapWidth(width), mapHeight(height), tileSize(tileSize) {
    map.resize(mapHeight / tileSize);
    for (auto& row : map) {
        row.resize(mapWidth / tileSize, 0);
    }

    int tilesetWidth, tilesetHeight;
    SDL_QueryTexture(tileset, nullptr, nullptr, &tilesetWidth, &tilesetHeight);
    printf("Value is %d, Value is %d\n", tilesetWidth, tilesetHeight);

    for (int y = 0; y < tilesetHeight; y += tileSize) {
        for (int x = 0; x < tilesetWidth; x += tileSize) {
            SDL_Rect rect = { x, y, tileSize, tileSize };
            tileRects.push_back(rect);
        }
    }

    generateTerrain();
}

void TileMap::generateTerrain() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> tileTypeDist(0, tileRects.size() - 1);
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

    for (int y = 0; y < map.size(); ++y) {
        for (int x = 0; x < map[y].size(); ++x) {
            float prob = probDist(gen);
            if (prob < 0.01f) {
                map[y][x] = 2;
            } else if (prob < 0.1f) {
                map[y][x] = 1;
            }
        }
    }
}

void TileMap::render(SDL_Renderer* renderer, const Camera& camera) {
    if (!tileset) {
        std::cout << "Warning: TileMap has nullptr tileset texture." << std::endl;
        return;
    }

    SDL_Rect viewport = camera.getViewport();
    int startX = viewport.x / tileSize;
    int startY = viewport.y / tileSize;
    int endX = (viewport.x + viewport.w) / tileSize + 1;
    int endY = (viewport.y + viewport.h) / tileSize + 1;

    startX = std::max(0, startX);
    startY = std::max(0, startY);
    endX = std::min(static_cast<int>(map[0].size()), endX);
    endY = std::min(static_cast<int>(map.size()), endY);

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            int tileIndex = map[y][x];
            SDL_Rect destRect = { x * tileSize - viewport.x, y * tileSize - viewport.y, tileSize, tileSize };
            SDL_RenderCopy(renderer, tileset, &tileRects[tileIndex], &destRect);
        }
    }
}

bool TileMap::isObstacle(int x, int y) const {
    if (x < 0 || y < 0 || y >= map.size() || x >= map[y].size()) {
        return true;
    }
    return map[y][x] == 2;
}

bool TileMap::checkCollision(const Rectangle& rect) const {
    int startTileX = static_cast<int>(rect.x) / tileSize;
    int startTileY = static_cast<int>(rect.y) / tileSize;
    int endTileX = static_cast<int>(rect.x + rect.w) / tileSize;
    int endTileY = static_cast<int>(rect.y + rect.h) / tileSize;

    for (int y = startTileY; y <= endTileY; ++y) {
        for (int x = startTileX; x <= endTileX; ++x) {
            if (isObstacle(x, y)) {
                std::cout << "Collision detected at tile (" << x << ", " << y
                          << ") for rect at (" << rect.x << ", " << rect.y << ")" << std::endl;
                return true;
            }
        }
    }
    return false;
}
