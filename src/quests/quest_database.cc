#include "quests/quest_database.h"

QuestDatabase::QuestDatabase() {
  QuestDef goblinHunt;
  goblinHunt.id = 1;
  goblinHunt.name = "Goblin Menace";
  goblinHunt.description = "Thin out the goblins near the starting zone.";
  goblinHunt.category = QuestCategory::Bounty;
  goblinHunt.minLevel = 1;
  goblinHunt.maxLevel = 10;
  goblinHunt.giverNpcName = "Town Guide";
  goblinHunt.turnInNpcName = "Town Guide";
  goblinHunt.objectives.push_back(
      QuestObjectiveDef{QuestObjectiveType::KillMobType, MobType::Goblin, 0, "", 3});
  goblinHunt.reward.gold = 25;
  goblinHunt.reward.experience = 50;
  quests.emplace(goblinHunt.id, goblinHunt);
}

const QuestDef* QuestDatabase::getQuest(int id) const {
  auto it = quests.find(id);
  if (it == quests.end()) {
    return nullptr;
  }
  return &it->second;
}
