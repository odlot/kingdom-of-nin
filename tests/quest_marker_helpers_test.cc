#include "ecs/component/quest_log_component.h"
#include "quests/quest_database.h"
#include "ui/quest_marker_helpers.h"
#include "world/map.h"
#include "world/region.h"
#include "world/tile.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    failures += 1;
  }
}

bool nearlyEqual(float a, float b, float epsilon = 0.01f) {
  return std::fabs(a - b) <= epsilon;
}
} // namespace

int main() {
  std::unordered_map<Coordinate, Tile> tiles;
  const int width = 10;
  const int height = 10;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      tiles.emplace(Coordinate(x, y), Tile::Grass);
    }
  }
  std::vector<Region> regions;
  regions.emplace_back(RegionType::StartingZone, 0, 0, 5, 5);
  Map map(width, height, std::move(tiles), regions, Coordinate(0, 0));

  QuestDatabase database;
  QuestLogComponent log;
  QuestProgress progress;
  progress.questId = 1;
  progress.completed = true;
  progress.rewardsClaimed = false;
  QuestObjectiveProgress objective;
  objective.def.type = QuestObjectiveType::EnterRegion;
  objective.def.regionName = "Starting Zone";
  objective.def.requiredCount = 1;
  objective.currentCount = 0;
  progress.objectives.push_back(objective);
  log.activeQuests.push_back(progress);

  std::vector<NpcMarkerInfo> npcs;
  npcs.push_back(NpcMarkerInfo{"Town Guide", Position(64.0f, 64.0f)});

  const std::vector<MinimapMarker> markers =
      buildQuestMinimapMarkers(log, database, map, npcs);
  expect(markers.size() == 2, "quest minimap markers include turn-in + region");

  bool foundTurnIn = false;
  bool foundRegion = false;
  for (const MinimapMarker& marker : markers) {
    if (marker.color.r == 255 && marker.color.g == 220 && marker.color.b == 80) {
      foundTurnIn = nearlyEqual(marker.tileX, 2.0f) && nearlyEqual(marker.tileY, 2.0f);
    }
    if (marker.color.r == 120 && marker.color.g == 220 && marker.color.b == 255) {
      foundRegion = nearlyEqual(marker.tileX, 2.5f) && nearlyEqual(marker.tileY, 2.5f);
    }
  }
  expect(foundTurnIn, "turn-in marker appears at NPC location");
  expect(foundRegion, "region marker appears at region center");

  const std::vector<Position> turnInMarkers =
      buildQuestTurnInWorldMarkers(log, database, npcs);
  expect(turnInMarkers.size() == 1, "world turn-in marker emitted");
  if (!turnInMarkers.empty()) {
    expect(nearlyEqual(turnInMarkers.front().x, 64.0f) &&
               nearlyEqual(turnInMarkers.front().y, 64.0f),
           "world marker matches NPC position");
  }

  if (failures > 0) {
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
  }
  std::cout << "All quest marker helper tests passed.\n";
  return EXIT_SUCCESS;
}
