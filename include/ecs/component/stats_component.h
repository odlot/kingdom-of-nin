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
  int unspentPoints = 0;
};
