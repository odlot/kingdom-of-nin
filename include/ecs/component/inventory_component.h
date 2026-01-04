#pragma once

#include <vector>

#include "component.h"
#include "items/item.h"

class InventoryComponent : public Component {
public:
  std::vector<ItemInstance> items;
};
