#pragma once

#include "SDL3/SDL.h"
#include "ecs/position.h"
#include "world/map.h"

class Minimap {
public:
  Minimap(int width, int height, int margin);

  void render(SDL_Renderer* renderer, const Map& map, const Position& playerPosition,
              int windowWidth, int windowHeight);

private:
  int width;
  int height;
  int margin;
};
