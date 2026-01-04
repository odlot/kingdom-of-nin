#pragma once

#include "component.h"
#include "ecs/position.h"
#include <SDL3/SDL.h>

class GraphicComponent : public Component {
public:
  GraphicComponent(Position position, SDL_Color color) : position(position), color(color) {}

public:
  Position position;
  SDL_Color color;
};