#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/health_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/mana_component.h"

class CharacterStats {
public:
  CharacterStats() = default;

  void render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth,
              const HealthComponent& health, const ManaComponent& mana, const LevelComponent& level,
              int attackPower, bool isVisible) const;
};
