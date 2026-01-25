#include "quests/quest_helpers.h"

std::vector<QuestEntry> buildNpcQuestEntries(const std::string& npcName,
                                             const QuestSystem& questSystem,
                                             const QuestDatabase& questDatabase,
                                             const QuestLogComponent& questLog,
                                             const LevelComponent& level) {
  std::vector<QuestEntry> entries;
  const std::vector<const QuestDef*> available =
      questSystem.availableQuests(npcName, level, questLog);
  for (const QuestDef* def : available) {
    entries.push_back(QuestEntry{QuestEntryType::Available, def});
  }
  for (const QuestProgress& progress : questLog.activeQuests) {
    const QuestDef* def = questDatabase.getQuest(progress.questId);
    if (!def || def->turnInNpcName != npcName) {
      continue;
    }
    if (progress.completed && !progress.rewardsClaimed) {
      entries.push_back(QuestEntry{QuestEntryType::TurnIn, def});
    }
  }
  return entries;
}

std::string buildNpcDialogText(const std::string& baseLine,
                               const std::vector<QuestEntry>& entries,
                               int selectionIndex) {
  std::string text = baseLine;
  if (entries.empty()) {
    return text;
  }
  text += "\n\nQuests:";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const QuestEntry& entry = entries[i];
    const QuestDef* def = entry.def;
    if (!def) {
      continue;
    }
    const std::string prefix =
        (static_cast<int>(i) == selectionIndex) ? "> " : "  ";
    const std::string type =
        (entry.type == QuestEntryType::Available) ? "[Accept]" : "[Turn In]";
    text += "\n" + prefix + def->name + " " + type;
  }
  text += "\n\nPageUp/PageDown to select";
  text += "\nPress A to accept, C to turn in";
  return text;
}
