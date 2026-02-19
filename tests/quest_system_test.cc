#include "ecs/component/inventory_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/quest_log_component.h"
#include "ecs/component/stats_component.h"
#include "events/event_bus.h"
#include "quests/quest_database.h"
#include "quests/quest_system.h"
#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    failures += 1;
  }
}
} // namespace

int main() {
  QuestDatabase database;
  QuestSystem system(database);

  {
    QuestLogComponent log;
    LevelComponent level(1, 0, 100);
    system.addQuest(log, level, 1);
    expect(log.activeQuests.size() == 1, "quest added at valid level");
    system.addQuest(log, level, 1);
    expect(log.activeQuests.size() == 1, "duplicate quest not added");

    QuestLogComponent gatedLog;
    LevelComponent gatedLevel(20, 0, 100);
    system.addQuest(gatedLog, gatedLevel, 1);
    expect(gatedLog.activeQuests.empty(), "quest gated by max level");
  }

  {
    QuestLogComponent log;
    LevelComponent level(1, 0, 100);
    StatsComponent stats;
    InventoryComponent inventory;
    system.addQuest(log, level, 1);

    EventBus bus;
    bus.emitMobKilledEvent(MobKilledEvent{MobType::Goblin, 1});
    bus.emitMobKilledEvent(MobKilledEvent{MobType::Goblin, 2});
    bus.emitMobKilledEvent(MobKilledEvent{MobType::Goblin, 3});
    system.update(bus, log, stats, level, inventory);

    expect(!log.activeQuests.empty(), "quest exists after update");
    expect(log.activeQuests.front().completed, "quest completes after kills");
    expect(!log.activeQuests.front().rewardsClaimed, "rewards not auto-claimed");

    const std::vector<std::string> wrongTurnIn =
        system.tryTurnIn("Shopkeeper", log, stats, level, inventory);
    expect(wrongTurnIn.empty(), "turn-in ignored for wrong NPC");

    const std::vector<std::string> turnedIn =
        system.tryTurnIn("Town Guide", log, stats, level, inventory);
    expect(turnedIn.size() == 1, "turn-in succeeds at correct NPC");
    expect(log.activeQuests.front().rewardsClaimed, "rewards claimed on turn-in");
    expect(stats.gold >= 25, "gold reward granted");
    expect(level.experience >= 50, "experience reward granted");
  }

  {
    QuestLogComponent log;
    LevelComponent level(1, 0, 100);
    const std::vector<const QuestDef*> available =
        system.availableQuests("Town Guide", level, log);
    expect(!available.empty(), "available quests returned for NPC");
  }

  if (failures > 0) {
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
  }
  std::cout << "All quest system tests passed.\n";
  return EXIT_SUCCESS;
}
