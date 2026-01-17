#pragma once

#include <unordered_map>

#include "skills/skill.h"

class SkillDatabase {
public:
  SkillDatabase();

  const SkillDef* getSkill(int id) const;

private:
  void addSkill(SkillDef def);

  std::unordered_map<int, SkillDef> skills;
};
