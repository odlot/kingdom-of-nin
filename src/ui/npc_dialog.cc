#include "ui/npc_dialog.h"

#include "ui/text_utils.h"
#include <algorithm>
#include <cmath>
#include <vector>

void renderNpcDialog(SDL_Renderer* renderer, TTF_Font* font, const std::string& title,
                     const std::string& text, int windowWidth, int windowHeight,
                     float& scroll) {
  SDL_Color titleColor = {255, 240, 210, 255};
  SDL_Color textColor = {235, 235, 235, 255};
  constexpr float panelWidth = 360.0f;
  constexpr float panelHeight = 120.0f;
  constexpr float panelPadding = 10.0f;
  constexpr float titleOffsetY = 10.0f;
  constexpr float textOffsetY = 34.0f;
  constexpr float lineHeight = 16.0f;
  const float textAreaHeight = panelHeight - textOffsetY - panelPadding;
  const int maxWidth = static_cast<int>(panelWidth - (panelPadding * 2.0f));
  const std::vector<std::string> lines = wrapText(font, text, maxWidth);
  const int totalLines = static_cast<int>(lines.size());
  const int maxVisibleLines =
      std::max(1, static_cast<int>(std::floor(textAreaHeight / lineHeight)));
  const float totalHeight = totalLines * lineHeight;
  const float visibleHeight = maxVisibleLines * lineHeight;
  const float maxScroll = std::max(0.0f, totalHeight - visibleHeight);
  scroll = std::clamp(scroll, 0.0f, maxScroll);
  const int startLine = static_cast<int>(std::floor(scroll / lineHeight));
  const float lineOffset = std::fmod(scroll, lineHeight);
  const int endLine = std::min(totalLines, startLine + maxVisibleLines + 1);

  SDL_Surface* titleSurface =
      TTF_RenderText_Solid(font, title.c_str(), title.length(), titleColor);
  SDL_FRect panel = {12.0f, windowHeight - panelHeight - 92.0f, panelWidth, panelHeight};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 10, 10, 10, 210);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 140);
  SDL_RenderRect(renderer, &panel);

  SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
  SDL_FRect titleRect = {panel.x + panelPadding, panel.y + titleOffsetY,
                         static_cast<float>(titleSurface->w),
                         static_cast<float>(titleSurface->h)};
  SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);

  float textY = panel.y + textOffsetY - lineOffset;
  for (int i = startLine; i < endLine; ++i) {
    SDL_Surface* lineSurface =
        TTF_RenderText_Solid(font, lines[i].c_str(), lines[i].length(), textColor);
    SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
    SDL_FRect lineRect = {panel.x + panelPadding, textY,
                          static_cast<float>(lineSurface->w),
                          static_cast<float>(lineSurface->h)};
    if (lineRect.y + lineRect.h >= panel.y + textOffsetY - 2.0f &&
        lineRect.y <= panel.y + panelHeight - panelPadding) {
      SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
    }
    SDL_DestroyTexture(lineTexture);
    SDL_DestroySurface(lineSurface);
    textY += lineHeight;
  }

  if (maxScroll > 0.0f) {
    const float trackX = panel.x + panel.w - 8.0f;
    const float trackY = panel.y + textOffsetY;
    const float trackH = visibleHeight;
    const float thumbH = std::max(18.0f, (visibleHeight / totalHeight) * trackH);
    const float scrollRatio = (maxScroll > 0.0f) ? (scroll / maxScroll) : 0.0f;
    const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
    SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
    SDL_RenderFillRect(renderer, &trackRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 220);
    SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
    SDL_RenderFillRect(renderer, &thumbRect);
  }

  SDL_DestroyTexture(titleTexture);
  SDL_DestroySurface(titleSurface);
}
