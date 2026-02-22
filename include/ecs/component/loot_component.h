#pragma once

#include "component.h"

class LootComponent : public Component {
public:
  explicit LootComponent(int itemId = 0, float despawnSeconds = 90.0f)
      : itemId(itemId), despawnSeconds(despawnSeconds) {}

  int itemId;
  float ageSeconds = 0.0f;
  float despawnSeconds = 90.0f;
};
