#pragma once

#include <string>

#include "ecs/component/mob_component.h"
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

struct MobKilledEvent {
  MobType mobType = MobType::Goblin;
  int mobEntityId = -1;
};

struct ItemPickupEvent {
  int itemId = 0;
  int amount = 1;
};
