#pragma once

#include <unordered_set>

#include "component.h"

class SkillTreeComponent : public Component {
public:
  int unspentPoints = 0;
  std::unordered_set<int> unlockedSkills;
};
