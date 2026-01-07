#pragma once

#include "component.h"

class LootComponent : public Component {
public:
  explicit LootComponent(int itemId = 0) : itemId(itemId) {}

  int itemId;
};
