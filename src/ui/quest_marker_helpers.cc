#include "ui/quest_marker_helpers.h"

#include <optional>

namespace {
std::string regionName(RegionType type) {
  switch (type) {
  case RegionType::StartingZone:
    return "Starting Zone";
  case RegionType::SpawnRegion:
    return "Spawn Region";
  case RegionType::GoblinCamp:
    return "Goblin Camp";
  case RegionType::DungeonEntrance:
    return "Dungeon Entrance";
  }
  return "Region";
}

std::optional<Position> regionCenterByName(const Map& map, const std::string& name) {
  const std::vector<Region>& regions = map.getRegions();
  for (const Region& region : regions) {
    if (regionName(region.type) == name) {
      const float centerX = (region.x + (region.width / 2.0f)) * TILE_SIZE;
      const float centerY = (region.y + (region.height / 2.0f)) * TILE_SIZE;
      return Position(centerX, centerY);
    }
  }
  return std::nullopt;
}
} // namespace

std::vector<MinimapMarker> buildQuestMinimapMarkers(const QuestLogComponent& questLog,
                                                    const QuestDatabase& questDatabase,
                                                    const Map& map,
                                                    const std::vector<NpcMarkerInfo>& npcs) {
  std::vector<MinimapMarker> markers;
  for (const QuestProgress& progress : questLog.activeQuests) {
    const QuestDef* def = questDatabase.getQuest(progress.questId);
    if (!def) {
      continue;
    }
    if (!def->turnInNpcName.empty() && progress.completed && !progress.rewardsClaimed) {
      for (const NpcMarkerInfo& npc : npcs) {
        if (npc.name == def->turnInNpcName) {
          markers.push_back(MinimapMarker{npc.position.x / TILE_SIZE,
                                          npc.position.y / TILE_SIZE,
                                          SDL_Color{255, 220, 80, 255}});
        }
      }
    }
    for (const QuestObjectiveProgress& objective : progress.objectives) {
      if (objective.def.type != QuestObjectiveType::EnterRegion) {
        continue;
      }
      if (objective.currentCount >= objective.def.requiredCount) {
        continue;
      }
      const std::optional<Position> regionCenter =
          regionCenterByName(map, objective.def.regionName);
      if (regionCenter.has_value()) {
        markers.push_back(
            MinimapMarker{regionCenter->x / TILE_SIZE, regionCenter->y / TILE_SIZE,
                          SDL_Color{120, 220, 255, 255}});
      }
    }
  }
  return markers;
}

std::vector<Position> buildQuestTurnInWorldMarkers(const QuestLogComponent& questLog,
                                                   const QuestDatabase& questDatabase,
                                                   const std::vector<NpcMarkerInfo>& npcs) {
  std::vector<Position> markers;
  for (const QuestProgress& progress : questLog.activeQuests) {
    const QuestDef* def = questDatabase.getQuest(progress.questId);
    if (!def || def->turnInNpcName.empty() || !progress.completed || progress.rewardsClaimed) {
      continue;
    }
    for (const NpcMarkerInfo& npc : npcs) {
      if (npc.name == def->turnInNpcName) {
        markers.push_back(npc.position);
      }
    }
  }
  return markers;
}
