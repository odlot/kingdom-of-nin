#pragma once

#include <string>

struct SkillDef {
  int id = 0;
  std::string name;
  float cooldown = 0.0f;
  float buffDuration = 0.0f;
};
