#include "skills/skill_database.h"

SkillDatabase::SkillDatabase() {
  SkillDef slash;
  slash.id = 1;
  slash.name = "Slash";
  slash.cooldown = 0.5f;
  addSkill(slash);

  SkillDef haste;
  haste.id = 2;
  haste.name = "Haste";
  haste.cooldown = 10.0f;
  haste.buffDuration = 6.0f;
  addSkill(haste);

  SkillDef rage;
  rage.id = 3;
  rage.name = "Rage";
  rage.cooldown = 12.0f;
  rage.buffDuration = 8.0f;
  addSkill(rage);

  SkillDef whirlwind;
  whirlwind.id = 4;
  whirlwind.name = "Whirlwind";
  whirlwind.cooldown = 6.0f;
  addSkill(whirlwind);

  SkillDef guard;
  guard.id = 5;
  guard.name = "Guard";
  guard.cooldown = 8.0f;
  guard.buffDuration = 4.0f;
  addSkill(guard);

  SkillDef focus;
  focus.id = 6;
  focus.name = "Focus";
  focus.cooldown = 10.0f;
  focus.buffDuration = 5.0f;
  addSkill(focus);
}

const SkillDef* SkillDatabase::getSkill(int id) const {
  auto it = skills.find(id);
  if (it == skills.end()) {
    return nullptr;
  }
  return &it->second;
}

void SkillDatabase::addSkill(SkillDef def) {
  skills.emplace(def.id, std::move(def));
}
