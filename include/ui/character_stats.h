#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/stats_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/mana_component.h"

class CharacterStats {
public:
  CharacterStats() = default;

  void handleInput(int mouseX, int mouseY, bool mousePressed,
                   StatsComponent& stats, bool isVisible);
  void render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth,
              const HealthComponent& health, const ManaComponent& mana,
              const LevelComponent& level, int attackPower, int strength,
              int dexterity, int intellect, int luck, int unspentPoints,
              bool isVisible) const;

private:
  bool wasMousePressed = false;
};
