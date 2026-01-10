#pragma once

#include "component.h"

enum class MobType { Goblin, GoblinArcher, GoblinBrute };

class MobComponent : public Component {
public:
  MobComponent(MobType type, int level, int homeX, int homeY, int regionX, int regionY,
               int regionWidth, int regionHeight, float aggroRange, float leashRange, float speed,
               int experience, int attackDamage, float attackCooldown, float attackRange)
      : level(level), homeX(homeX), homeY(homeY), regionX(regionX), regionY(regionY),
        regionWidth(regionWidth), regionHeight(regionHeight), aggroRange(aggroRange),
        leashRange(leashRange), speed(speed), experience(experience), type(type),
        attackDamage(attackDamage), attackCooldown(attackCooldown), attackRange(attackRange) {}

  int level;
  int homeX;
  int homeY;
  int regionX;
  int regionY;
  int regionWidth;
  int regionHeight;
  float aggroRange;
  float leashRange;
  float speed;
  int experience;
  MobType type;
  int attackDamage;
  float attackCooldown;
  float attackRange;
  float attackTimer = 0.0f;
};
