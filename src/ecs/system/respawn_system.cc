#include "ecs/system/respawn_system.h"

#include <algorithm>
#include <optional>

#include "ecs/component/collision_component.h"
#include "ecs/component/graphic_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/mob_component.h"
#include "ecs/component/pushback_component.h"
#include "ecs/component/transform_component.h"
#include "world/tile.h"

namespace {
constexpr int SPAWN_REGION_MOB_COUNT = 5;
constexpr float MOB_RESPAWN_SECONDS = 6.0f;
constexpr float MOB_SPAWN_ANIMATION_SECONDS = 0.6f;
constexpr int MOB_LEVEL_CAP = 60;

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

int spawnLevelForRegion(const Region& region) {
  if (region.type != RegionType::SpawnRegion) {
    return 1;
  }
  const int levelBand = std::max(0, (region.x + region.y) / 64);
  const int computed = 1 + (levelBand * 5);
  return std::clamp(computed, 1, MOB_LEVEL_CAP);
}

int spawnMob(Registry& registry, const Position& position, const Region& region,
             std::vector<int>& mobEntityIds, const MobDatabase& mobDatabase,
             const MobArchetype& archetype, int mobLevel) {
  const MobResolvedStats resolved = mobDatabase.resolveStats(archetype.type, mobLevel);
  int mobEntityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), mobEntityId);
  registry.registerComponentForEntity<GraphicComponent>(
      std::make_unique<GraphicComponent>(position, resolved.color), mobEntityId);
  registry.registerComponentForEntity<CollisionComponent>(
      std::make_unique<CollisionComponent>(32.0f, 32.0f, false), mobEntityId);
  registry.registerComponentForEntity<PushbackComponent>(
      std::make_unique<PushbackComponent>(0.0f, 0.0f, 0.0f), mobEntityId);
  registry.registerComponentForEntity<HealthComponent>(
      std::make_unique<HealthComponent>(resolved.maxHealth, resolved.maxHealth), mobEntityId);
  const int tileX = static_cast<int>(position.x / TILE_SIZE);
  const int tileY = static_cast<int>(position.y / TILE_SIZE);
  registry.registerComponentForEntity<MobComponent>(
      std::make_unique<MobComponent>(archetype.type, resolved.level, tileX, tileY, region.x,
                                     region.y, region.width, region.height, resolved.aggroRange,
                                     resolved.leashRange, resolved.speed, resolved.experience,
                                     resolved.attackDamage, resolved.attackCooldown,
                                     resolved.attackRange),
      mobEntityId);
  mobEntityIds.push_back(mobEntityId);
  return mobEntityId;
}

void resetMob(Registry& registry, int entityId, const Position& position, const Region& region,
              const MobDatabase& mobDatabase, const MobArchetype& archetype, int mobLevel) {
  const MobResolvedStats resolved = mobDatabase.resolveStats(archetype.type, mobLevel);
  TransformComponent& transform = registry.getComponent<TransformComponent>(entityId);
  GraphicComponent& graphic = registry.getComponent<GraphicComponent>(entityId);
  HealthComponent& health = registry.getComponent<HealthComponent>(entityId);
  MobComponent& mob = registry.getComponent<MobComponent>(entityId);

  transform.position = position;
  graphic.color = resolved.color;
  const int tileX = static_cast<int>(position.x / TILE_SIZE);
  const int tileY = static_cast<int>(position.y / TILE_SIZE);
  mob.homeX = tileX;
  mob.homeY = tileY;
  mob.regionX = region.x;
  mob.regionY = region.y;
  mob.regionWidth = region.width;
  mob.regionHeight = region.height;
  mob.type = archetype.type;
  mob.level = resolved.level;
  mob.aggroRange = resolved.aggroRange;
  mob.leashRange = resolved.leashRange;
  mob.speed = resolved.speed;
  mob.experience = resolved.experience;
  mob.attackDamage = resolved.attackDamage;
  mob.attackCooldown = resolved.attackCooldown;
  mob.attackRange = resolved.attackRange;
  mob.attackTimer = 0.0f;
  health.max = resolved.maxHealth;
  health.current = resolved.maxHealth;
}
} // namespace

RespawnSystem::RespawnSystem(const MobDatabase& mobDatabase, unsigned int seed)
    : mobDatabase(mobDatabase), rng(seed) {}

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
    SpawnRegionState state{region, spawnLevelForRegion(region),
                           std::vector<SpawnSlot>(SPAWN_REGION_MOB_COUNT)};
    for (SpawnSlot& slot : state.slots) {
      std::optional<Position> spawnPosition = randomSpawnPosition(map, region, rng);
      if (!spawnPosition.has_value()) {
        slot.entityId = -1;
        slot.respawnTimer = MOB_RESPAWN_SECONDS;
        continue;
      }
      const MobArchetype& archetype = this->mobDatabase.randomArchetype(rng);
      slot.entityId = spawnMob(registry, *spawnPosition, region, mobEntityIds, this->mobDatabase,
                               archetype, state.mobLevel);
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
            const MobArchetype& archetype = this->mobDatabase.randomArchetype(rng);
            slot.entityId = spawnMob(registry, *spawnPosition, spawnRegion.region, mobEntityIds,
                                     this->mobDatabase, archetype, spawnRegion.mobLevel);
          } else {
            const MobArchetype& archetype = this->mobDatabase.randomArchetype(rng);
            resetMob(registry, slot.entityId, *spawnPosition, spawnRegion.region, this->mobDatabase,
                     archetype, spawnRegion.mobLevel);
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
