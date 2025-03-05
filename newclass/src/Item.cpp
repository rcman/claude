#include "Item.h"

Item::Item(Vector2D pos, SDL_Texture* tex, ItemType t, int val, const std::string& n) :
    Entity(pos, tex, TILE_SIZE, TILE_SIZE), type(t), value(val), name(n) {}

ItemType Item::getType() const { return type; }
int Item::getValue() const { return value; }
std::string Item::getName() const { return name; }
