#pragma once

#include "SDL3/SDL.h"
#include "ecs/position.h"
#include "system.h"

class GraphicSystem : public System {
public:
  GraphicSystem(Registry& registry, std::bitset<MAX_COMPONENTS> signature);
  void render(SDL_Renderer* renderer, const Position& cameraPosition);
};
