#ifndef TILEMAP_H
#define TILEMAP_H

#include "Common.h"
#include "Camera.h"
#include "Rectangle.h"

class TileMap {
private:
    std::vector<std::vector<int>> map;
    SDL_Texture* tileset;
    std::vector<SDL_Rect> tileRects;
    int mapWidth, mapHeight;
    int tileSize;

public:
    TileMap(SDL_Texture* tiles, int width, int height, int tileSize);
    void generateTerrain();
    void render(SDL_Renderer* renderer, const Camera& camera);
    bool isObstacle(int x, int y) const;
    bool checkCollision(const Rectangle& rect) const;
};

#endif // TILEMAP_H
