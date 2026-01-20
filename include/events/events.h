#pragma once

#include <string>

#include "ecs/position.h"

struct DamageEvent {
  int attackerId = -1;
  int targetId = -1;
  int amount = 0;
  Position targetPosition;
};

enum class FloatingTextKind { Damage = 0, CritDamage, Heal, Info };

struct FloatingTextEvent {
  std::string text;
  Position position;
  FloatingTextKind kind = FloatingTextKind::Info;
};

enum class RegionTransition { Enter, Leave };

struct RegionEvent {
  RegionTransition transition;
  std::string regionName;
  Position position;
};
