#pragma once

#include <string>
#include <vector>

#include "ecs/component/mob_component.h"

enum class QuestObjectiveType { KillMobType = 0, CollectItem, EnterRegion };

enum class QuestCategory { Main = 0, Side, Bounty };

struct QuestObjectiveDef {
  QuestObjectiveType type = QuestObjectiveType::KillMobType;
  MobType mobType = MobType::Goblin;
  int itemId = 0;
  std::string regionName;
  int requiredCount = 1;
};

struct QuestReward {
  int gold = 0;
  int experience = 0;
  int itemId = 0;
};

struct QuestDef {
  int id = 0;
  std::string name;
  std::string description;
  QuestCategory category = QuestCategory::Side;
  int minLevel = 0;
  int maxLevel = 0;
  std::string giverNpcName;
  std::string turnInNpcName;
  std::vector<QuestObjectiveDef> objectives;
  QuestReward reward;
};

struct QuestObjectiveProgress {
  QuestObjectiveDef def;
  int currentCount = 0;
};

struct QuestProgress {
  int questId = 0;
  std::vector<QuestObjectiveProgress> objectives;
  bool completed = false;
  bool rewardsClaimed = false;
};
