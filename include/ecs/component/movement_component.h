#pragma once

#include "component.h"

class MovementComponent : public Component {
public:
  MovementComponent(float speed = 0.0f) : speed(speed) {}

public:
  float speed;
};