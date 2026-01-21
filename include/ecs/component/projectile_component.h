#pragma once

#include "SDL3/SDL_pixels.h"
#include "ecs/component/component.h"

class ProjectileComponent : public Component {
public:
  ProjectileComponent(int sourceEntityId, int targetEntityId, float velocityX, float velocityY,
                      float remainingRange, int damage, bool isCrit, float radius,
                      SDL_Color color)
      : sourceEntityId(sourceEntityId),
        targetEntityId(targetEntityId),
        velocityX(velocityX),
        velocityY(velocityY),
        remainingRange(remainingRange),
        damage(damage),
        isCrit(isCrit),
        radius(radius),
        color(color) {}

  int sourceEntityId = -1;
  int targetEntityId = -1;
  float velocityX = 0.0f;
  float velocityY = 0.0f;
  float lastX = 0.0f;
  float lastY = 0.0f;
  float remainingRange = 0.0f;
  int damage = 0;
  bool isCrit = false;
  float radius = 4.0f;
  SDL_Color color = {255, 255, 255, 255};
};
