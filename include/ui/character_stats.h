#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

#include "ecs/component/health_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/mana_component.h"
#include "ecs/component/stats_component.h"

class CharacterStats {
public:
  CharacterStats() = default;

  void handleInput(int mouseX, int mouseY, bool mousePressed, int windowWidth,
                   StatsComponent& stats, bool isVisible);
  void render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth,
              const HealthComponent& health, const ManaComponent& mana, const LevelComponent& level,
              int attackPower, const std::string& className, int strength, int gold, int dexterity,
              int intellect, int luck, int unspentPoints, bool isVisible) const;
  static SDL_FRect panelRectForTesting(int windowWidth);
  static SDL_FRect plusRectForTesting(const SDL_FRect& panel, int index);

private:
  bool wasMousePressed = false;
};
