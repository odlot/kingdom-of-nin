#pragma once

#include "SDL3/SDL.h"
#include "ecs/position.h"
#include "world/map.h"
#include <vector>

struct MinimapMarker {
  float tileX = 0.0f;
  float tileY = 0.0f;
  SDL_Color color = {255, 255, 255, 255};
};

class Minimap {
public:
  Minimap(int width, int height, int margin);

  void render(SDL_Renderer* renderer, const Map& map, const Position& playerPosition,
              int windowWidth, int windowHeight, const std::vector<MinimapMarker>& markers);

private:
  int width;
  int height;
  int margin;
};
