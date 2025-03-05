#include "Inventory.h"

Inventory::Inventory(int cap) : capacity(cap) {
    items[WEAPON] = 0;
    items[ARMOR] = 0;
    items[HEALTH] = 0;
    items[RESOURCE_WOOD] = 0;
    items[RESOURCE_METAL] = 0;
    items[RESOURCE_FOOD] = 0;
    items[RESOURCE_CLOTH] = 0;
    items[AMMO] = 0;
}

bool Inventory::addItem(const Item& item) {
    if (getTotalItems() >= capacity) return false;

    items[item.getType()] += item.getValue();
    return true;
}

bool Inventory::useItem(ItemType type, int amount) {
    if (items[type] < amount) return false;

    items[type] -= amount;
    return true;
}

int Inventory::getItemCount(ItemType type) const {
    auto it = items.find(type);
    if (it != items.end()) {
        return it->second;
    }
    return 0;
}

int Inventory::getTotalItems() const {
    int total = 0;
    for (const auto& item : items) {
        total += item.second;
    }
    return total;
}

void Inventory::transferTo(Inventory& other, ItemType type, int amount) {
    int available = std::min(amount, items[type]);
    if (available <= 0) return;

    if (other.getTotalItems() + available <= other.capacity) {
        items[type] -= available;
        other.items[type] += available;
    }
}
