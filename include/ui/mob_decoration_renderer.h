#pragma once

#include <vector>

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/position.h"

class Registry;
class RespawnSystem;

class MobDecorationRenderer {
public:
  void render(SDL_Renderer* renderer, TTF_Font* font, Registry& registry,
              RespawnSystem& respawnSystem, const std::vector<int>& mobEntityIds,
              const Position& playerCenter, const Position& cameraPosition) const;
};
