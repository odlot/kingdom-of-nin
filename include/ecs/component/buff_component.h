#pragma once

#include <string>
#include <vector>

#include "component.h"

struct BuffInstance {
  int id = 0;
  std::string name;
  float remaining = 0.0f;
  float duration = 0.0f;
};

class BuffComponent : public Component {
public:
  std::vector<BuffInstance> buffs;
};
