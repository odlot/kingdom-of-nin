#pragma once

#include <random>
#include <unordered_map>

#include "items/item.h"

struct EquipmentDropGenerationOptions {
  int weaponWeight = 35;
  int armorWeight = 65;
  int commonWeight = 70;
  int rareWeight = 25;
  int epicWeight = 5;
};

class ItemDatabase {
public:
  ItemDatabase();

  const ItemDef* getItem(int id) const;
  int generateEquipmentDrop(int targetLevel, CharacterClass preferredClass, std::mt19937& rng,
                            const EquipmentDropGenerationOptions& options = {});

private:
  void addItem(ItemDef def);

  int nextGeneratedItemId = 1000;
  std::unordered_map<int, ItemDef> items;
};
