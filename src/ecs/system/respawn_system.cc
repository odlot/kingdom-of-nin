#include "ecs/system/respawn_system.h"

#include <algorithm>
#include <optional>

#include "ecs/component/collision_component.h"
#include "ecs/component/graphic_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/mob_component.h"
#include "ecs/component/transform_component.h"
#include "world/tile.h"

namespace {
constexpr int SPAWN_REGION_MOB_COUNT = 5;
constexpr float MOB_RESPAWN_SECONDS = 6.0f;
constexpr float MOB_SPAWN_ANIMATION_SECONDS = 0.6f;
constexpr float MOB_AGGRO_RANGE = 160.0f;
constexpr float MOB_LEASH_RANGE = 220.0f;
constexpr float MOB_SPEED = 60.0f;

std::optional<Coordinate> firstWalkableInRegion(const Map& map, const Region& region) {
  for (int y = region.y; y < region.y + region.height; ++y) {
    for (int x = region.x; x < region.x + region.width; ++x) {
      if (map.isWalkable(x, y)) {
        return Coordinate(x, y);
      }
    }
  }
  return std::nullopt;
}

std::optional<Position> randomSpawnPosition(const Map& map, const Region& region,
                                            std::mt19937& rng) {
  if (region.width <= 0 || region.height <= 0) {
    return std::nullopt;
  }
  std::uniform_int_distribution<int> distX(region.x, region.x + region.width - 1);
  std::uniform_int_distribution<int> distY(region.y, region.y + region.height - 1);
  for (int attempt = 0; attempt < 30; ++attempt) {
    const int tileX = distX(rng);
    const int tileY = distY(rng);
    if (!map.isWalkable(tileX, tileY)) {
      continue;
    }
    return Position(tileX * TILE_SIZE, tileY * TILE_SIZE);
  }
  std::optional<Coordinate> fallback = firstWalkableInRegion(map, region);
  if (!fallback.has_value()) {
    return std::nullopt;
  }
  return Position(fallback->x * TILE_SIZE, fallback->y * TILE_SIZE);
}

int spawnGoblin(Registry& registry, const Position& position, const Region& region,
                std::vector<int>& mobEntityIds) {
  int mobEntityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), mobEntityId);
  registry.registerComponentForEntity<GraphicComponent>(
      std::make_unique<GraphicComponent>(position, SDL_Color({40, 200, 80, 255})), mobEntityId);
  registry.registerComponentForEntity<CollisionComponent>(
      std::make_unique<CollisionComponent>(32.0f, 32.0f, false), mobEntityId);
  registry.registerComponentForEntity<HealthComponent>(std::make_unique<HealthComponent>(20, 20),
                                                       mobEntityId);
  const int tileX = static_cast<int>(position.x / TILE_SIZE);
  const int tileY = static_cast<int>(position.y / TILE_SIZE);
  registry.registerComponentForEntity<MobComponent>(
      std::make_unique<MobComponent>(1, tileX, tileY, region.x, region.y, region.width,
                                     region.height, MOB_AGGRO_RANGE, MOB_LEASH_RANGE, MOB_SPEED),
      mobEntityId);
  mobEntityIds.push_back(mobEntityId);
  return mobEntityId;
}

void resetGoblin(Registry& registry, int entityId, const Position& position, const Region& region,
                 SDL_Color color) {
  TransformComponent& transform = registry.getComponent<TransformComponent>(entityId);
  GraphicComponent& graphic = registry.getComponent<GraphicComponent>(entityId);
  HealthComponent& health = registry.getComponent<HealthComponent>(entityId);
  MobComponent& mob = registry.getComponent<MobComponent>(entityId);

  transform.position = position;
  graphic.color = color;
  health.current = health.max;
  const int tileX = static_cast<int>(position.x / TILE_SIZE);
  const int tileY = static_cast<int>(position.y / TILE_SIZE);
  mob.homeX = tileX;
  mob.homeY = tileY;
  mob.regionX = region.x;
  mob.regionY = region.y;
  mob.regionWidth = region.width;
  mob.regionHeight = region.height;
}
} // namespace

RespawnSystem::RespawnSystem() : rng(std::random_device{}()) {}

void RespawnSystem::beginSpawnAnimation(int entityId, GraphicComponent& graphic, float duration) {
  SpawnAnimation animation;
  animation.remaining = duration;
  animation.duration = duration;
  animation.baseColor = graphic.color;
  animation.baseColor.a = 255;
  graphic.color.a = 0;
  spawnAnimations[entityId] = animation;
}

void RespawnSystem::initialize(const Map& map, Registry& registry, std::vector<int>& mobEntityIds) {
  spawnRegions.clear();
  spawnAnimations.clear();
  for (const Region& region : map.getRegions()) {
    if (region.type != RegionType::SpawnRegion) {
      continue;
    }
    SpawnRegionState state{region, std::vector<SpawnSlot>(SPAWN_REGION_MOB_COUNT)};
    for (SpawnSlot& slot : state.slots) {
      std::optional<Position> spawnPosition = randomSpawnPosition(map, region, rng);
      if (!spawnPosition.has_value()) {
        slot.entityId = -1;
        slot.respawnTimer = MOB_RESPAWN_SECONDS;
        continue;
      }
      slot.entityId = spawnGoblin(registry, *spawnPosition, region, mobEntityIds);
      slot.respawnTimer = 0.0f;
      GraphicComponent& graphic = registry.getComponent<GraphicComponent>(slot.entityId);
      beginSpawnAnimation(slot.entityId, graphic, MOB_SPAWN_ANIMATION_SECONDS);
    }
    spawnRegions.push_back(std::move(state));
  }
}

void RespawnSystem::update(float dt, const Map& map, Registry& registry,
                           std::vector<int>& mobEntityIds) {
  for (auto it = spawnAnimations.begin(); it != spawnAnimations.end();) {
    SpawnAnimation& animation = it->second;
    animation.remaining = std::max(0.0f, animation.remaining - dt);
    GraphicComponent& graphic = registry.getComponent<GraphicComponent>(it->first);
    const float progress =
        animation.duration > 0.0f ? (1.0f - (animation.remaining / animation.duration)) : 1.0f;
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    SDL_Color color = animation.baseColor;
    color.a = static_cast<Uint8>(static_cast<float>(animation.baseColor.a) * clamped);
    graphic.color = color;
    if (animation.remaining <= 0.0f) {
      graphic.color = animation.baseColor;
      it = spawnAnimations.erase(it);
    } else {
      ++it;
    }
  }

  for (SpawnRegionState& spawnRegion : spawnRegions) {
    for (SpawnSlot& slot : spawnRegion.slots) {
      if (slot.respawnTimer > 0.0f) {
        slot.respawnTimer = std::max(0.0f, slot.respawnTimer - dt);
        if (slot.respawnTimer <= 0.0f) {
          std::optional<Position> spawnPosition = randomSpawnPosition(map, spawnRegion.region, rng);
          if (!spawnPosition.has_value()) {
            slot.respawnTimer = MOB_RESPAWN_SECONDS;
            continue;
          }
          if (slot.entityId < 0) {
            slot.entityId = spawnGoblin(registry, *spawnPosition, spawnRegion.region, mobEntityIds);
          } else {
            resetGoblin(registry, slot.entityId, *spawnPosition, spawnRegion.region,
                        SDL_Color({40, 200, 80, 255}));
          }
          GraphicComponent& graphic = registry.getComponent<GraphicComponent>(slot.entityId);
          beginSpawnAnimation(slot.entityId, graphic, MOB_SPAWN_ANIMATION_SECONDS);
        }
        continue;
      }
      if (slot.entityId < 0) {
        slot.respawnTimer = MOB_RESPAWN_SECONDS;
        continue;
      }
      const HealthComponent& health = registry.getComponent<HealthComponent>(slot.entityId);
      if (health.current <= 0) {
        slot.respawnTimer = MOB_RESPAWN_SECONDS;
      }
    }
  }
}

bool RespawnSystem::isSpawning(int entityId) const {
  auto it = spawnAnimations.find(entityId);
  return it != spawnAnimations.end() && it->second.remaining > 0.0f;
}
