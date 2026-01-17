#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/buff_component.h"

class BuffBar {
public:
  void render(SDL_Renderer* renderer, TTF_Font* font, const BuffComponent& buffs);
};
