#include "items/item_database.h"

ItemDatabase::ItemDatabase() {
  ItemDef basicSword;
  basicSword.id = 1;
  basicSword.name = "Basic Sword";
  basicSword.slot = ItemSlot::Weapon;
  basicSword.requiredLevel = 1;
  basicSword.allowedClasses = {CharacterClass::Any};
  basicSword.stats.attackPower = 5;
  addItem(basicSword);
}

const ItemDef* ItemDatabase::getItem(int id) const {
  auto it = items.find(id);
  if (it == items.end()) {
    return nullptr;
  }
  return &it->second;
}

void ItemDatabase::addItem(ItemDef def) {
  items.emplace(def.id, std::move(def));
}
