#pragma once

#include <vector>

#include "ecs/component/component.h"
#include "quests/quest.h"

class QuestLogComponent : public Component {
public:
  std::vector<QuestProgress> activeQuests;
};
