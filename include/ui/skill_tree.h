#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/skill_tree_component.h"
#include "skills/skill_database.h"
#include "skills/skill_tree.h"

class SkillTree {
public:
  void handleInput(const bool* keyboardState, int mouseX, int mouseY, bool mousePressed,
                   SkillTreeComponent& tree, const SkillTreeDefinition& definition,
                   int windowWidth);
  void render(SDL_Renderer* renderer, TTF_Font* font, const SkillTreeComponent& tree,
              const SkillTreeDefinition& definition, const SkillDatabase& database,
              int windowWidth, int windowHeight);
  bool isOpen() const { return isOpenFlag; }

private:
  bool wasTogglePressed = false;
  bool wasMousePressed = false;
  bool isOpenFlag = false;
  int lastMouseX = 0;
  int lastMouseY = 0;
};
