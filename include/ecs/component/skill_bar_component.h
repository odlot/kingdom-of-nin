#pragma once

#include <array>

#include "component.h"

struct SkillSlot {
  int skillId = -1;
  float cooldownRemaining = 0.0f;
};

class SkillBarComponent : public Component {
public:
  static constexpr std::size_t kSlotCount = 5;

  SkillBarComponent() : slots{} {}

  std::array<SkillSlot, kSlotCount> slots;
};
