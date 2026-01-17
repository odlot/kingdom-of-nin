#include "ecs/system/pushback_system.h"

#include "ecs/component/collision_component.h"
#include "ecs/component/pushback_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/registry.h"
#include "world/map.h"
#include "world/tile.h"

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

PushbackSystem::PushbackSystem(Registry& registry, std::bitset<MAX_COMPONENTS> signature)
    : System(registry, signature) {}

void PushbackSystem::update(float dt, const Map& map) {
  for (auto it = this->entityIdsBegin(); it != this->entityIdsEnd(); ++it) {
    int entityId = *it;
    TransformComponent& transform = registry.getComponent<TransformComponent>(entityId);
    const CollisionComponent& collision = registry.getComponent<CollisionComponent>(entityId);
    PushbackComponent& pushback = registry.getComponent<PushbackComponent>(entityId);
    if (pushback.remaining <= 0.0f) {
      continue;
    }

    const float step = std::min(dt, pushback.remaining);
    const float newX = transform.position.x + (pushback.velocityX * step);
    const float newY = transform.position.y + (pushback.velocityY * step);

    if (!isBlockedByMap(map, collision, newX, transform.position.y)) {
      transform.position.x = newX;
    }
    if (!isBlockedByMap(map, collision, transform.position.x, newY)) {
      transform.position.y = newY;
    }

    pushback.remaining = std::max(0.0f, pushback.remaining - step);
  }
}
