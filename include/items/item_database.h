#pragma once

#include <random>
#include <unordered_map>

#include "items/item.h"

class ItemDatabase {
public:
  ItemDatabase();

  const ItemDef* getItem(int id) const;
  int generateEquipmentDrop(int targetLevel, CharacterClass preferredClass, std::mt19937& rng);

private:
  void addItem(ItemDef def);

  int nextGeneratedItemId = 1000;
  std::unordered_map<int, ItemDef> items;
};
