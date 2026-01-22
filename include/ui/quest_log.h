#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/quest_log_component.h"
#include "items/item_database.h"
#include "quests/quest_database.h"

class QuestLog {
public:
  QuestLog() = default;

  SDL_FRect panelRect(int windowWidth, int windowHeight) const;

  void render(SDL_Renderer* renderer, TTF_Font* font, const QuestLogComponent& questLog,
              const QuestDatabase& questDatabase, const ItemDatabase& itemDatabase,
              int windowWidth, int windowHeight, bool isVisible, float& scroll) const;
};
