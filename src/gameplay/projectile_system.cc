#include "gameplay/projectile_system.h"

#include "ecs/component/collision_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/projectile_component.h"
#include "ecs/component/transform_component.h"
#include "ui/render_utils.h"
#include <algorithm>
#include <cmath>

namespace {
Position centerForEntity(const TransformComponent& transform, const CollisionComponent& collision) {
  return Position(transform.position.x + (collision.width / 2.0f),
                  transform.position.y + (collision.height / 2.0f));
}

float squaredDistance(const Position& a, const Position& b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return (dx * dx) + (dy * dy);
}
} // namespace

void updateProjectiles(float dt, Registry& registry, const Map& map,
                       RespawnSystem& respawnSystem, std::vector<int>& projectileEntityIds,
                       int playerEntityId, const ProjectileHitFn& onHit) {
  const TransformComponent& playerTransform =
      registry.getComponent<TransformComponent>(playerEntityId);
  const CollisionComponent& playerCollision =
      registry.getComponent<CollisionComponent>(playerEntityId);
  const Position playerCenter = centerForEntity(playerTransform, playerCollision);

  for (std::size_t i = 0; i < projectileEntityIds.size();) {
    const int projectileId = projectileEntityIds[i];
    TransformComponent& projectileTransform =
        registry.getComponent<TransformComponent>(projectileId);
    ProjectileComponent& projectile =
        registry.getComponent<ProjectileComponent>(projectileId);
    bool shouldRemove = false;

    projectile.lastX = projectileTransform.position.x;
    projectile.lastY = projectileTransform.position.y;
    const float dx = projectile.velocityX * dt;
    const float dy = projectile.velocityY * dt;
    projectileTransform.position.x += dx;
    projectileTransform.position.y += dy;
    projectile.remainingRange =
        std::max(0.0f, projectile.remainingRange - std::sqrt((dx * dx) + (dy * dy)));

    const int tileX = static_cast<int>(projectileTransform.position.x / TILE_SIZE);
    const int tileY = static_cast<int>(projectileTransform.position.y / TILE_SIZE);
    if (!map.isWalkable(tileX, tileY) || projectile.remainingRange <= 0.0f) {
      shouldRemove = true;
    }

    if (!shouldRemove && projectile.targetEntityId != -1) {
      const HealthComponent& mobHealth =
          registry.getComponent<HealthComponent>(projectile.targetEntityId);
      if (mobHealth.current <= 0 || respawnSystem.isSpawning(projectile.targetEntityId)) {
        shouldRemove = true;
      } else {
        const TransformComponent& mobTransform =
            registry.getComponent<TransformComponent>(projectile.targetEntityId);
        const CollisionComponent& mobCollision =
            registry.getComponent<CollisionComponent>(projectile.targetEntityId);
        const Position mobCenter = centerForEntity(mobTransform, mobCollision);
        const float radius = (mobCollision.width / 2.0f) + projectile.radius;
        if (squaredDistance(projectileTransform.position, mobCenter) <= radius * radius) {
          if (onHit) {
            onHit(projectile.targetEntityId, projectile.damage, projectile.isCrit, mobCenter,
                  playerCenter);
          }
          shouldRemove = true;
        }
      }
    }

    if (shouldRemove) {
      projectileTransform.position = Position(-1000.0f, -1000.0f);
      projectileEntityIds.erase(projectileEntityIds.begin() + i);
    } else {
      ++i;
    }
  }
}

void renderProjectiles(SDL_Renderer* renderer, const Position& cameraPosition, Registry& registry,
                       const std::vector<int>& projectileEntityIds) {
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  for (int projectileId : projectileEntityIds) {
    const TransformComponent& projectileTransform =
        registry.getComponent<TransformComponent>(projectileId);
    const ProjectileComponent& projectile =
        registry.getComponent<ProjectileComponent>(projectileId);
    const float size = projectile.radius * 2.0f;
    SDL_FRect projectileRect = {projectileTransform.position.x - projectile.radius -
                                    cameraPosition.x,
                                projectileTransform.position.y - projectile.radius -
                                    cameraPosition.y,
                                size, size};
    SDL_SetRenderDrawColor(renderer, projectile.color.r, projectile.color.g, projectile.color.b,
                           projectile.color.a);
    SDL_RenderFillRect(renderer, &projectileRect);
    if (projectile.trailLength > 0.0f) {
      const float speed = std::sqrt((projectile.velocityX * projectile.velocityX) +
                                    (projectile.velocityY * projectile.velocityY));
      if (speed > 0.001f) {
        const float nx = projectile.velocityX / speed;
        const float ny = projectile.velocityY / speed;
        const float tailX = projectileTransform.position.x - (nx * projectile.trailLength);
        const float tailY = projectileTransform.position.y - (ny * projectile.trailLength);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        SDL_RenderLine(renderer, tailX - cameraPosition.x, tailY - cameraPosition.y,
                       projectileTransform.position.x - cameraPosition.x,
                       projectileTransform.position.y - cameraPosition.y);
      }
    }
    drawCircle(renderer, projectileTransform.position, projectile.radius + 2.0f, cameraPosition,
               SDL_Color{255, 255, 255, 200});
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 200);
    SDL_RenderRect(renderer, &projectileRect);
  }
}
