#pragma once

#include "component.h"

class CollisionComponent : public Component {
public:
  CollisionComponent(float width = 0.0f, float height = 0.0f, bool isSolid = true)
      : width(width), height(height), isSolid(isSolid) {}

public:
  float width;
  float height;
  bool isSolid;
};
