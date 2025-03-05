#ifndef BUILDING_H
#define BUILDING_H

#include "Entity.h"
#include "Item.h"
#include "Inventory.h"

class Building : public Entity {
private:
    std::vector<Item*> items;
    bool isPlayerHome;
    Rectangle entrance;
    Rectangle interior;
    Inventory storage;

public:
    Building(Vector2D pos, SDL_Texture* tex, int w, int h);
    bool isAtEntrance(const Vector2D& pos) const;
    bool isInside(const Vector2D& pos) const;
    void addItem(Item* item);
    Item* getItem(int index);
    std::vector<Item*>& getItems();
    bool isHomeBase() const;
    void setHomeBase(bool home);
    Inventory& getStorage();
};

#endif // BUILDING_H
