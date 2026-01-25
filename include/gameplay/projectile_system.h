#pragma once

#include <functional>
#include <vector>

#include "SDL3/SDL_render.h"
#include "ecs/position.h"
#include "ecs/registry.h"
#include "ecs/system/respawn_system.h"
#include "world/map.h"

using ProjectileHitFn =
    std::function<void(int mobEntityId, int damage, bool isCrit, const Position& hitPosition,
                       const Position& fromPosition)>;

void updateProjectiles(float dt, Registry& registry, const Map& map, RespawnSystem& respawnSystem,
                       std::vector<int>& projectileEntityIds, int playerEntityId,
                       const ProjectileHitFn& onHit);

void renderProjectiles(SDL_Renderer* renderer, const Position& cameraPosition,
                       Registry& registry, const std::vector<int>& projectileEntityIds);
