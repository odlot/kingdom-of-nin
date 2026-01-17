#include "skills/skill_tree.h"

SkillTreeDefinition::SkillTreeDefinition() {
  SkillNodeDef slash;
  slash.id = 1;
  slash.row = 0;
  slash.col = 1;
  addNode(slash);

  SkillNodeDef haste;
  haste.id = 2;
  haste.row = 1;
  haste.col = 0;
  haste.prerequisites = {1};
  addNode(haste);

  SkillNodeDef rage;
  rage.id = 3;
  rage.row = 1;
  rage.col = 2;
  rage.prerequisites = {1};
  addNode(rage);

  SkillNodeDef whirlwind;
  whirlwind.id = 4;
  whirlwind.row = 2;
  whirlwind.col = 1;
  whirlwind.prerequisites = {2, 3};
  addNode(whirlwind);

  SkillNodeDef guard;
  guard.id = 5;
  guard.row = 3;
  guard.col = 0;
  guard.prerequisites = {2};
  addNode(guard);

  SkillNodeDef focus;
  focus.id = 6;
  focus.row = 3;
  focus.col = 2;
  focus.prerequisites = {3};
  addNode(focus);
}

const SkillNodeDef* SkillTreeDefinition::getNode(int id) const {
  for (const SkillNodeDef& node : nodes_) {
    if (node.id == id) {
      return &node;
    }
  }
  return nullptr;
}

void SkillTreeDefinition::addNode(SkillNodeDef node) {
  nodes_.push_back(std::move(node));
}
