#pragma once

#include <string>
#include <vector>

#include "ecs/component/quest_log_component.h"
#include "quests/quest_database.h"
#include "quests/quest_system.h"

enum class QuestEntryType { Available = 0, TurnIn };

struct QuestEntry {
  QuestEntryType type = QuestEntryType::Available;
  const QuestDef* def = nullptr;
};

std::vector<QuestEntry> buildNpcQuestEntries(const std::string& npcName,
                                             const QuestSystem& questSystem,
                                             const QuestDatabase& questDatabase,
                                             const QuestLogComponent& questLog,
                                             const LevelComponent& level);

std::string buildNpcDialogText(const std::string& baseLine,
                               const std::vector<QuestEntry>& entries,
                               int selectionIndex);
