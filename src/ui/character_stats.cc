#include "ui/character_stats.h"

#include <array>
#include <string>

namespace {
constexpr float STATS_PANEL_WIDTH = 260.0f;
constexpr float STATS_PANEL_HEIGHT = 220.0f;
constexpr float LINE_HEIGHT = 16.0f;
constexpr float TITLE_Y_OFFSET = 6.0f;
constexpr float LIST_Y_OFFSET = 28.0f;
constexpr int STAT_LINE_START = 6;
constexpr float PLUS_BOX_SIZE = 12.0f;

SDL_FRect statsPanelRect(int windowWidth) {
  SDL_FRect panel = {16.0f, 48.0f, STATS_PANEL_WIDTH, STATS_PANEL_HEIGHT};
  if (windowWidth > 0) {
    panel.x = windowWidth - STATS_PANEL_WIDTH - 12.0f;
  }
  return panel;
}

SDL_FRect statPlusRect(const SDL_FRect& panel, int index) {
  float y = panel.y + LIST_Y_OFFSET + (index * LINE_HEIGHT);
  return SDL_FRect{panel.x + panel.w - 22.0f, y + 2.0f, PLUS_BOX_SIZE,
                   PLUS_BOX_SIZE};
}

bool pointInRect(int x, int y, const SDL_FRect& rect) {
  return x >= rect.x && x <= rect.x + rect.w && y >= rect.y &&
         y <= rect.y + rect.h;
}
} // namespace

void CharacterStats::handleInput(int mouseX, int mouseY, bool mousePressed,
                                 StatsComponent& stats, bool isVisible) {
  const bool click = mousePressed && !wasMousePressed;
  wasMousePressed = mousePressed;
  if (!click || !isVisible || stats.unspentPoints <= 0) {
    return;
  }

  SDL_FRect panel = statsPanelRect(0);
  const SDL_FRect plusRects[4] = {
      statPlusRect(panel, STAT_LINE_START + 0),
      statPlusRect(panel, STAT_LINE_START + 1),
      statPlusRect(panel, STAT_LINE_START + 2),
      statPlusRect(panel, STAT_LINE_START + 3),
  };

  for (int i = 0; i < 4; ++i) {
    if (!pointInRect(mouseX, mouseY, plusRects[i])) {
      continue;
    }
    switch (i) {
    case 0:
      stats.strength += 1;
      break;
    case 1:
      stats.dexterity += 1;
      break;
    case 2:
      stats.intellect += 1;
      break;
    case 3:
      stats.luck += 1;
      break;
    }
    stats.unspentPoints -= 1;
    break;
  }
}

void CharacterStats::render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth,
                            const HealthComponent& health, const ManaComponent& mana,
                            const LevelComponent& level, int attackPower, int strength,
                            int dexterity, int intellect, int luck, int unspentPoints,
                            bool isVisible) const {
  if (!isVisible) {
    return;
  }

  SDL_Color textColor = {255, 255, 255, 255};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_FRect panel = statsPanelRect(windowWidth);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
  SDL_RenderRect(renderer, &panel);

  SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Character Stats", 15, textColor);
  SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
  SDL_FRect titleRect = {panel.x + 8.0f, panel.y + TITLE_Y_OFFSET,
                         static_cast<float>(titleSurface->w), static_cast<float>(titleSurface->h)};
  SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);
  SDL_DestroySurface(titleSurface);
  SDL_DestroyTexture(titleTexture);

  const std::array<std::string, 11> lines = {
      "Level: " + std::to_string(level.level),
      "HP: " + std::to_string(health.current) + "/" + std::to_string(health.max),
      "MP: " + std::to_string(mana.current) + "/" + std::to_string(mana.max),
      "AP: " + std::to_string(attackPower),
      "XP: " + std::to_string(level.experience) + "/" + std::to_string(level.nextLevelExperience),
      "Class: Adventurer",
      "STR: " + std::to_string(strength),
      "DEX: " + std::to_string(dexterity),
      "INT: " + std::to_string(intellect),
      "LUK: " + std::to_string(luck),
      "Points: " + std::to_string(unspentPoints),
  };

  float textY = panel.y + LIST_Y_OFFSET;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& line = lines[i];
    SDL_Surface* lineSurface =
        TTF_RenderText_Solid(font, line.c_str(), static_cast<int>(line.size()), textColor);
    SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
    SDL_FRect lineRect = {panel.x + 8.0f, textY, static_cast<float>(lineSurface->w),
                          static_cast<float>(lineSurface->h)};
    SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
    SDL_DestroySurface(lineSurface);
    SDL_DestroyTexture(lineTexture);

    if (unspentPoints > 0 && i >= STAT_LINE_START && i < STAT_LINE_START + 4) {
      SDL_FRect plusRect = statPlusRect(panel, static_cast<int>(i));
      SDL_SetRenderDrawColor(renderer, 70, 70, 70, 220);
      SDL_RenderFillRect(renderer, &plusRect);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
      SDL_RenderRect(renderer, &plusRect);

      SDL_Surface* plusSurface =
          TTF_RenderText_Solid(font, "+", 1, textColor);
      SDL_Texture* plusTexture =
          SDL_CreateTextureFromSurface(renderer, plusSurface);
      SDL_FRect plusTextRect = {plusRect.x + 3.0f, plusRect.y - 1.0f,
                                static_cast<float>(plusSurface->w),
                                static_cast<float>(plusSurface->h)};
      SDL_RenderTexture(renderer, plusTexture, nullptr, &plusTextRect);
      SDL_DestroySurface(plusSurface);
      SDL_DestroyTexture(plusTexture);
    }
    textY += LINE_HEIGHT;
  }
}
