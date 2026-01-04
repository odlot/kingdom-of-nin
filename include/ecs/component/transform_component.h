#pragma once

#include "component.h"
#include "ecs/position.h"

class TransformComponent : public Component {
public:
  TransformComponent(Position position) : position(position) {}

public:
  Position position;
};