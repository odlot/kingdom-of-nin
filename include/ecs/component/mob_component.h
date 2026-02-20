#pragma once

#include "component.h"

enum class MobType {
  Goblin,
  GoblinArcher,
  GoblinBrute,
  Skeleton,
  SkeletonArcher,
  Necromancer,
  Wolf,
  DireWolf,
  Bandit,
  BanditArcher,
  BanditBruiser,
  Slime,
  ArcaneWisp,
  ArcaneSentinel,
  Ogre
};

inline const char* mobTypeName(MobType type) {
  switch (type) {
  case MobType::Goblin:
    return "Goblin";
  case MobType::GoblinArcher:
    return "Goblin Archer";
  case MobType::GoblinBrute:
    return "Goblin Brute";
  case MobType::Skeleton:
    return "Skeleton";
  case MobType::SkeletonArcher:
    return "Skeleton Archer";
  case MobType::Necromancer:
    return "Necromancer";
  case MobType::Wolf:
    return "Wolf";
  case MobType::DireWolf:
    return "Dire Wolf";
  case MobType::Bandit:
    return "Bandit";
  case MobType::BanditArcher:
    return "Bandit Archer";
  case MobType::BanditBruiser:
    return "Bandit Bruiser";
  case MobType::Slime:
    return "Slime";
  case MobType::ArcaneWisp:
    return "Arcane Wisp";
  case MobType::ArcaneSentinel:
    return "Arcane Sentinel";
  case MobType::Ogre:
    return "Ogre";
  }
  return "Mob";
}

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
