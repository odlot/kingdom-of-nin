#pragma once

#include <string>

#include "ecs/position.h"

struct DamageEvent {
  int attackerId = -1;
  int targetId = -1;
  int amount = 0;
  Position targetPosition;
};

struct FloatingTextEvent {
  std::string text;
  Position position;
};
