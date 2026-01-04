#include "ui/character_stats.h"

#include <array>
#include <string>

namespace {
constexpr float STATS_PANEL_WIDTH = 260.0f;
constexpr float STATS_PANEL_HEIGHT = 220.0f;
constexpr float LINE_HEIGHT = 16.0f;
constexpr float TITLE_Y_OFFSET = 6.0f;
constexpr float LIST_Y_OFFSET = 28.0f;

SDL_FRect statsPanelRect(int windowWidth) {
  SDL_FRect panel = {16.0f, 48.0f, STATS_PANEL_WIDTH, STATS_PANEL_HEIGHT};
  if (windowWidth > 0) {
    panel.x = windowWidth - STATS_PANEL_WIDTH - 12.0f;
  }
  return panel;
}
} // namespace

void CharacterStats::render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth,
                            const HealthComponent& health, const ManaComponent& mana,
                            const LevelComponent& level, int attackPower, bool isVisible) const {
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

  const std::array<std::string, 6> lines = {
      "Level: " + std::to_string(level.level),
      "HP: " + std::to_string(health.current) + "/" + std::to_string(health.max),
      "MP: " + std::to_string(mana.current) + "/" + std::to_string(mana.max),
      "AP: " + std::to_string(attackPower),
      "XP: " + std::to_string(level.experience) + "/" + std::to_string(level.nextLevelExperience),
      "Class: Adventurer",
  };

  float textY = panel.y + LIST_Y_OFFSET;
  for (const std::string& line : lines) {
    SDL_Surface* lineSurface =
        TTF_RenderText_Solid(font, line.c_str(), static_cast<int>(line.size()), textColor);
    SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
    SDL_FRect lineRect = {panel.x + 8.0f, textY, static_cast<float>(lineSurface->w),
                          static_cast<float>(lineSurface->h)};
    SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
    SDL_DestroySurface(lineSurface);
    SDL_DestroyTexture(lineTexture);
    textY += LINE_HEIGHT;
  }
}
