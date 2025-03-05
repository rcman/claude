#ifndef ITEM_H
#define ITEM_H

#include "Entity.h"

class Item : public Entity {
private:
    ItemType type;
    int value;
    std::string name;

public:
    Item(Vector2D pos, SDL_Texture* tex, ItemType t, int val, const std::string& n);
    ItemType getType() const;
    int getValue() const;
    std::string getName() const;
};

#endif // ITEM_H
