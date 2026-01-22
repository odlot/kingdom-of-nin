#pragma once

#include <unordered_map>

#include "quests/quest.h"

class QuestDatabase {
public:
  QuestDatabase();

  const QuestDef* getQuest(int id) const;
  const std::unordered_map<int, QuestDef>& allQuests() const { return quests; }

private:
  std::unordered_map<int, QuestDef> quests;
};
