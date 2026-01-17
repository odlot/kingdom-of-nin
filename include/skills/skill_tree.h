#pragma once

#include <vector>

struct SkillNodeDef {
  int id = 0;
  int row = 0;
  int col = 0;
  std::vector<int> prerequisites;
};

class SkillTreeDefinition {
public:
  SkillTreeDefinition();

  const std::vector<SkillNodeDef>& nodes() const { return nodes_; }
  const SkillNodeDef* getNode(int id) const;

private:
  void addNode(SkillNodeDef node);

  std::vector<SkillNodeDef> nodes_;
};
