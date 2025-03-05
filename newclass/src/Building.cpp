#include "Building.h"

Building::Building(Vector2D pos, SDL_Texture* tex, int w, int h) :
    Entity(pos, tex, w, h), isPlayerHome(false) {
    entrance.x = position.x + w / 2 - TILE_SIZE / 2;
    entrance.y = position.y + h - TILE_SIZE;
    entrance.w = TILE_SIZE;
    entrance.h = TILE_SIZE;

    interior.x = position.x;
    interior.y = position.y;
    interior.w = w;
    interior.h = h;
}

bool Building::isAtEntrance(const Vector2D& pos) const {
    return entrance.contains(pos.x, pos.y);
}

bool Building::isInside(const Vector2D& pos) const {
    return interior.contains(pos.x, pos.y);
}

void Building::addItem(Item* item) {
    items.push_back(item);
}

Item* Building::getItem(int index) {
    if (index >= 0 && index < items.size()) {
        Item* item = items[index];
        items.erase(items.begin() + index);
        return item;
    }
    return nullptr;
}

std::vector<Item*>& Building::getItems() {
    return items;
}

bool Building::isHomeBase() const { return isPlayerHome; }
void Building::setHomeBase(bool home) { isPlayerHome = home; }

Inventory& Building::getStorage() { return storage; }
