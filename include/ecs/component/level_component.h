#pragma once

#include "component.h"

class LevelComponent : public Component {
public:
  LevelComponent(int level = 1, int experience = 0, int nextLevelExperience = 100)
      : level(level), experience(experience), nextLevelExperience(nextLevelExperience) {}

public:
  int level;
  int experience;
  int nextLevelExperience;
};
