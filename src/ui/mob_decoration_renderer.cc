#include "ui/mob_decoration_renderer.h"

#include <algorithm>
#include <string>

#include "ecs/component/collision_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/mob_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/registry.h"
#include "ecs/system/respawn_system.h"

namespace {
constexpr float kDecorationRange = 180.0f;
constexpr float kBarHeight = 4.0f;
constexpr float kBarGap = 2.0f;

Position centerForEntity(const TransformComponent& transform, const CollisionComponent& collision) {
  return Position(transform.position.x + (collision.width / 2.0f),
                  transform.position.y + (collision.height / 2.0f));
}

float squaredDistance(const Position& a, const Position& b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return (dx * dx) + (dy * dy);
}

void drawBar(SDL_Renderer* renderer, float x, float y, float width, float ratio,
             const SDL_Color& fillColor) {
  SDL_FRect bg = {x, y, width, kBarHeight};
  SDL_SetRenderDrawColor(renderer, 20, 20, 20, 220);
  SDL_RenderFillRect(renderer, &bg);

  SDL_FRect fill = bg;
  fill.w = width * std::clamp(ratio, 0.0f, 1.0f);
  SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  SDL_RenderFillRect(renderer, &fill);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 70);
  SDL_RenderRect(renderer, &bg);
}
} // namespace

void MobDecorationRenderer::render(SDL_Renderer* renderer, TTF_Font* font, Registry& registry,
                                   RespawnSystem& respawnSystem,
                                   const std::vector<int>& mobEntityIds,
                                   const Position& playerCenter,
                                   const Position& cameraPosition) const {
  if (!renderer || !font) {
    return;
  }

  const float rangeSquared = kDecorationRange * kDecorationRange;
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  for (int mobEntityId : mobEntityIds) {
    const HealthComponent& mobHealth = registry.getComponent<HealthComponent>(mobEntityId);
    if (mobHealth.current <= 0 || mobHealth.max <= 0) {
      continue;
    }
    if (respawnSystem.isSpawning(mobEntityId)) {
      continue;
    }

    const TransformComponent& mobTransform = registry.getComponent<TransformComponent>(mobEntityId);
    const CollisionComponent& mobCollision = registry.getComponent<CollisionComponent>(mobEntityId);
    const Position mobCenter = centerForEntity(mobTransform, mobCollision);
    if (squaredDistance(playerCenter, mobCenter) > rangeSquared) {
      continue;
    }

    const MobComponent& mob = registry.getComponent<MobComponent>(mobEntityId);
    const float barWidth = std::max(26.0f, mobCollision.width);
    float hpY = mobTransform.position.y - cameraPosition.y - 16.0f;
    if (mob.maxMana > 0.0f) {
      hpY -= (kBarHeight + kBarGap);
    }
    const float barX = mobCenter.x - cameraPosition.x - (barWidth / 2.0f);
    drawBar(renderer, barX, hpY, barWidth,
            static_cast<float>(mobHealth.current) / static_cast<float>(mobHealth.max),
            SDL_Color{210, 50, 50, 255});
    if (mob.maxMana > 0.0f) {
      drawBar(renderer, barX, hpY + kBarHeight + kBarGap, barWidth, mob.currentMana / mob.maxMana,
              SDL_Color{80, 130, 230, 255});
    }

    const std::string label =
        std::string(mobTypeName(mob.type)) + " Lv." + std::to_string(mob.level);
    SDL_Color textColor = {245, 245, 245, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, label.c_str(), label.length(), textColor);
    if (!surface) {
      continue;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
      SDL_DestroySurface(surface);
      continue;
    }

    SDL_FRect textRect = {mobCenter.x - cameraPosition.x - (static_cast<float>(surface->w) / 2.0f),
                          hpY - static_cast<float>(surface->h) - 4.0f,
                          static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_FRect bgRect = {textRect.x - 4.0f, textRect.y - 2.0f, textRect.w + 8.0f, textRect.h + 4.0f};
    SDL_RenderFillRect(renderer, &bgRect);
    SDL_RenderTexture(renderer, texture, nullptr, &textRect);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
  }
}
