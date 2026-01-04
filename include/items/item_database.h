#pragma once

#include <unordered_map>

#include "items/item.h"

class ItemDatabase {
public:
  ItemDatabase();

  const ItemDef* getItem(int id) const;

private:
  void addItem(ItemDef def);

  std::unordered_map<int, ItemDef> items;
};
