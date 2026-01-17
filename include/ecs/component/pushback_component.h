#pragma once

#include "component.h"

class PushbackComponent : public Component {
public:
  PushbackComponent(float velocityX, float velocityY, float duration)
      : velocityX(velocityX), velocityY(velocityY), remaining(duration) {}

  float velocityX = 0.0f;
  float velocityY = 0.0f;
  float remaining = 0.0f;
};
