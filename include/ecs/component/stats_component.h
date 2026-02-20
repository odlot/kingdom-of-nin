#pragma once

#include "component.h"

class StatsComponent : public Component {
public:
  int baseAttackPower = 1;
  int baseArmor = 0;
  int strength = 5;
  int dexterity = 5;
  int intellect = 5;
  int luck = 5;
  float critChanceBonus = 0.0f;
  float critDamageBonus = 0.0f;
  float accuracy = 0.0f;
  float dodge = 0.0f;
  float parry = 0.0f;
  int unspentPoints = 0;
  int gold = 50;
};
