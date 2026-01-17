#pragma once

#include "ecs/system/system.h"

class Map;

class PushbackSystem : public System {
public:
  PushbackSystem(Registry& registry, std::bitset<MAX_COMPONENTS> signature);
  void update(float dt, const Map& map);
};
