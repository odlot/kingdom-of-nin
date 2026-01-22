#include "quests/quest_system.h"

#include "ecs/component/inventory_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/stats_component.h"
#include "events/events.h"

namespace {
QuestProgress createProgress(const QuestDef& def) {
  QuestProgress progress;
  progress.questId = def.id;
  for (const QuestObjectiveDef& objective : def.objectives) {
    QuestObjectiveProgress entry;
    entry.def = objective;
    entry.currentCount = 0;
    progress.objectives.push_back(entry);
  }
  return progress;
}

bool isQuestActive(const QuestLogComponent& questLog, int questId) {
  for (const QuestProgress& quest : questLog.activeQuests) {
    if (quest.questId == questId) {
      return true;
    }
  }
  return false;
}
} // namespace

QuestSystem::QuestSystem(const QuestDatabase& database) : database(database) {}

void QuestSystem::addQuest(QuestLogComponent& questLog, const LevelComponent& level,
                           int questId) const {
  for (const QuestProgress& quest : questLog.activeQuests) {
    if (quest.questId == questId) {
      return;
    }
  }
  const QuestDef* def = database.getQuest(questId);
  if (!def) {
    return;
  }
  if (def->minLevel > 0 && level.level < def->minLevel) {
    return;
  }
  if (def->maxLevel > 0 && level.level > def->maxLevel) {
    return;
  }
  questLog.activeQuests.push_back(createProgress(*def));
}

void QuestSystem::update(EventBus& eventBus, QuestLogComponent& questLog, StatsComponent& stats,
                         LevelComponent& level, InventoryComponent& inventory) {
  const std::vector<MobKilledEvent> kills = eventBus.consumeMobKilledEvents();
  const std::vector<ItemPickupEvent> pickups = eventBus.consumeItemPickupEvents();
  const std::vector<RegionEvent> regions = eventBus.consumeRegionEvents();

  for (QuestProgress& quest : questLog.activeQuests) {
    if (quest.completed) {
      continue;
    }
    const QuestDef* def = database.getQuest(quest.questId);
    if (!def) {
      continue;
    }
    for (QuestObjectiveProgress& objective : quest.objectives) {
      if (objective.currentCount >= objective.def.requiredCount) {
        continue;
      }
      switch (objective.def.type) {
      case QuestObjectiveType::KillMobType: {
        for (const MobKilledEvent& kill : kills) {
          if (kill.mobType == objective.def.mobType) {
            objective.currentCount =
                std::min(objective.def.requiredCount, objective.currentCount + 1);
          }
        }
        break;
      }
      case QuestObjectiveType::CollectItem: {
        for (const ItemPickupEvent& pickup : pickups) {
          if (pickup.itemId == objective.def.itemId) {
            objective.currentCount =
                std::min(objective.def.requiredCount,
                         objective.currentCount + std::max(1, pickup.amount));
          }
        }
        break;
      }
      case QuestObjectiveType::EnterRegion: {
        for (const RegionEvent& region : regions) {
          if (region.transition == RegionTransition::Enter &&
              region.regionName == objective.def.regionName) {
            objective.currentCount = objective.def.requiredCount;
          }
        }
        break;
      }
      }
    }

    bool allDone = true;
    for (const QuestObjectiveProgress& objective : quest.objectives) {
      if (objective.currentCount < objective.def.requiredCount) {
        allDone = false;
        break;
      }
    }
    if (allDone) {
      quest.completed = true;
    }
  }

  for (QuestProgress& quest : questLog.activeQuests) {
    if (!quest.completed || quest.rewardsClaimed) {
      continue;
    }
    const QuestDef* def = database.getQuest(quest.questId);
    if (!def) {
      continue;
    }
    if (def->turnInNpcName.empty()) {
      stats.gold += def->reward.gold;
      level.experience += def->reward.experience;
      if (def->reward.itemId > 0) {
        inventory.addItem(ItemInstance{def->reward.itemId});
      }
      quest.rewardsClaimed = true;
    }
  }
}

std::vector<std::string> QuestSystem::tryTurnIn(const std::string& npcName,
                                                QuestLogComponent& questLog,
                                                StatsComponent& stats, LevelComponent& level,
                                                InventoryComponent& inventory) const {
  std::vector<std::string> completed;
  if (npcName.empty()) {
    return completed;
  }
  for (QuestProgress& quest : questLog.activeQuests) {
    if (!quest.completed || quest.rewardsClaimed) {
      continue;
    }
    const QuestDef* def = database.getQuest(quest.questId);
    if (!def) {
      continue;
    }
    if (def->turnInNpcName != npcName) {
      continue;
    }
    stats.gold += def->reward.gold;
    level.experience += def->reward.experience;
    if (def->reward.itemId > 0) {
      inventory.addItem(ItemInstance{def->reward.itemId});
    }
    quest.rewardsClaimed = true;
    completed.push_back(def->name);
  }
  return completed;
}

std::vector<const QuestDef*> QuestSystem::availableQuests(
    const std::string& npcName, const LevelComponent& level,
    const QuestLogComponent& questLog) const {
  std::vector<const QuestDef*> available;
  if (npcName.empty()) {
    return available;
  }
  for (const auto& entry : database.allQuests()) {
    const QuestDef& def = entry.second;
    if (def.giverNpcName != npcName) {
      continue;
    }
    if (def.minLevel > 0 && level.level < def.minLevel) {
      continue;
    }
    if (def.maxLevel > 0 && level.level > def.maxLevel) {
      continue;
    }
    if (isQuestActive(questLog, def.id)) {
      continue;
    }
    available.push_back(&def);
  }
  return available;
}
