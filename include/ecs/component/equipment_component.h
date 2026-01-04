#pragma once

#include <unordered_map>

#include "component.h"
#include "items/item.h"

class EquipmentComponent : public Component {
public:
  std::unordered_map<ItemSlot, ItemInstance> equipped;
};
