#ifndef INVENTORY_H
#define INVENTORY_H

#include "Common.h"
#include "Item.h"

class Inventory {
private:
    std::map<ItemType, int> items;
    int capacity;

public:
    Inventory(int cap = 20);
    bool addItem(const Item& item);
    bool useItem(ItemType type, int amount = 1);
    int getItemCount(ItemType type) const;
    int getTotalItems() const;
    void transferTo(Inventory& other, ItemType type, int amount);
};

#endif // INVENTORY_H
