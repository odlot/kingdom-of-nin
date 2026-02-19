#pragma once

#include "ecs/component/quest_log_component.h"
#include "ecs/position.h"
#include "quests/quest_database.h"
#include "ui/minimap.h"
#include "world/map.h"
#include <string>
#include <vector>

struct NpcMarkerInfo {
  std::string name;
  Position position;
};

std::vector<MinimapMarker> buildQuestMinimapMarkers(const QuestLogComponent& questLog,
                                                    const QuestDatabase& questDatabase,
                                                    const Map& map,
                                                    const std::vector<NpcMarkerInfo>& npcs);

std::vector<Position> buildQuestTurnInWorldMarkers(const QuestLogComponent& questLog,
                                                   const QuestDatabase& questDatabase,
                                                   const std::vector<NpcMarkerInfo>& npcs);
