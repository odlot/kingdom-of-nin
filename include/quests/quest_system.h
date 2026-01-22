#pragma once

#include "ecs/component/inventory_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/quest_log_component.h"
#include "ecs/component/stats_component.h"
#include "events/event_bus.h"
#include "quests/quest_database.h"

class QuestSystem {
public:
  explicit QuestSystem(const QuestDatabase& database);

  void update(EventBus& eventBus, QuestLogComponent& questLog, StatsComponent& stats,
              LevelComponent& level, InventoryComponent& inventory);

  void addQuest(QuestLogComponent& questLog, const LevelComponent& level, int questId) const;
  std::vector<const QuestDef*> availableQuests(const std::string& npcName,
                                               const LevelComponent& level,
                                               const QuestLogComponent& questLog) const;
  std::vector<std::string> tryTurnIn(const std::string& npcName, QuestLogComponent& questLog,
                                     StatsComponent& stats, LevelComponent& level,
                                     InventoryComponent& inventory) const;

private:
  const QuestDatabase& database;
};
