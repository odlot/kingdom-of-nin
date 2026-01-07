#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

#include "ecs/component/graphic_component.h"
#include "ecs/registry.h"
#include "world/map.h"
#include "world/region.h"

class RespawnSystem {
public:
  RespawnSystem();
  void initialize(const Map& map, Registry& registry, std::vector<int>& mobEntityIds);
  void update(float dt, const Map& map, Registry& registry, std::vector<int>& mobEntityIds);
  bool isSpawning(int entityId) const;

private:
  void beginSpawnAnimation(int entityId, GraphicComponent& graphic, float duration);

  struct SpawnSlot {
    int entityId = -1;
    float respawnTimer = 0.0f;
  };

  struct SpawnRegionState {
    Region region;
    std::vector<SpawnSlot> slots;
  };

  struct SpawnAnimation {
    float remaining = 0.0f;
    float duration = 0.0f;
    SDL_Color baseColor = {0, 0, 0, 255};
  };

  std::vector<SpawnRegionState> spawnRegions;
  std::unordered_map<int, SpawnAnimation> spawnAnimations;
  std::mt19937 rng;
};
