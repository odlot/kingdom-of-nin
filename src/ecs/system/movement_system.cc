#include "ecs/system/movement_system.h"
#include "ecs/component/collision_component.h"
#include "ecs/component/movement_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/registry.h"
#include "world/map.h"
#include "world/tile.h"

MovementSystem::MovementSystem(Registry& registry, std::bitset<MAX_COMPONENTS> signature)
    : System(registry, signature) {}

namespace {
bool isBlockedByMap(const Map& map, const CollisionComponent& collision, float nextX, float nextY) {
  const float left = nextX;
  const float right = nextX + collision.width - 1.0f;
  const float top = nextY;
  const float bottom = nextY + collision.height - 1.0f;

  const int tileLeft = static_cast<int>(left / TILE_SIZE);
  const int tileRight = static_cast<int>(right / TILE_SIZE);
  const int tileTop = static_cast<int>(top / TILE_SIZE);
  const int tileBottom = static_cast<int>(bottom / TILE_SIZE);

  if (!map.isWalkable(tileLeft, tileTop) || !map.isWalkable(tileRight, tileTop) ||
      !map.isWalkable(tileLeft, tileBottom) || !map.isWalkable(tileRight, tileBottom)) {
    return true;
  }
  return false;
}
} // namespace

void MovementSystem::update(std::pair<int, int> direction, float dt, const Map& map) {
  // Implementation of the movement system update logic
  for (auto it = this->entityIdsBegin(); it != this->entityIdsEnd(); ++it) {
    int entityId = *it;
    // Update each entity based on its components
    // This is a placeholder for actual movement logic
    TransformComponent& transformComponent = registry.getComponent<TransformComponent>(entityId);
    const MovementComponent& movementComponent = registry.getComponent<MovementComponent>(entityId);
    const CollisionComponent& collisionComponent =
        registry.getComponent<CollisionComponent>(entityId);
    float newX = transformComponent.position.x + movementComponent.speed * dt * direction.first;
    float newY = transformComponent.position.y + movementComponent.speed * dt * direction.second;

    if (!isBlockedByMap(map, collisionComponent, newX, transformComponent.position.y)) {
      transformComponent.position.x = newX;
    }
    if (!isBlockedByMap(map, collisionComponent, transformComponent.position.x, newY)) {
      transformComponent.position.y = newY;
    }
  }
}
