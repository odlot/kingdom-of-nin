#include "ui/buff_bar.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace {
constexpr float SLOT_SIZE = 28.0f;
constexpr float SLOT_PADDING = 6.0f;
constexpr float BAR_X = 12.0f;
constexpr float BAR_Y = 44.0f;
} // namespace

void BuffBar::render(SDL_Renderer* renderer, TTF_Font* font, const BuffComponent& buffs) {
  if (buffs.buffs.empty()) {
    return;
  }

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_Color textColor = {240, 240, 240, 255};
  float x = BAR_X;
  const float y = BAR_Y;
  for (const BuffInstance& buff : buffs.buffs) {
    SDL_FRect rect = {x, y, SLOT_SIZE, SLOT_SIZE};
    SDL_SetRenderDrawColor(renderer, 60, 120, 60, 220);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 200, 220, 200, 255);
    SDL_RenderRect(renderer, &rect);

    if (buff.duration > 0.0f && buff.remaining > 0.0f) {
      const float ratio = std::clamp(buff.remaining / buff.duration, 0.0f, 1.0f);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
      SDL_FRect cooldownRect = {rect.x, rect.y + (rect.h * (1.0f - ratio)), rect.w, rect.h * ratio};
      SDL_RenderFillRect(renderer, &cooldownRect);
    }

    const int seconds = static_cast<int>(std::ceil(buff.remaining));
    const std::string cdText = std::to_string(seconds);
    SDL_Surface* surface = TTF_RenderText_Solid(font, cdText.c_str(), cdText.length(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect textRect = {rect.x + (rect.w - surface->w) / 2.0f,
                          rect.y + (rect.h - surface->h) / 2.0f, static_cast<float>(surface->w),
                          static_cast<float>(surface->h)};
    SDL_RenderTexture(renderer, texture, nullptr, &textRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);

    x += SLOT_SIZE + SLOT_PADDING;
  }
}
