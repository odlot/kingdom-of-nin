#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/skill_bar_component.h"
#include "ecs/component/skill_tree_component.h"
#include "skills/skill_database.h"

class SkillBar {
public:
  void render(SDL_Renderer* renderer, TTF_Font* font, const SkillBarComponent& skills,
              const SkillTreeComponent& tree, const SkillDatabase& database, int windowWidth,
              int windowHeight);
};
