#pragma once

#include "system.h"

class Map;

class MovementSystem : public System {
public:
  MovementSystem(Registry& registry, std::bitset<MAX_COMPONENTS> signature);
  void update(std::pair<int, int> direction, float dt, const Map& map);
};
