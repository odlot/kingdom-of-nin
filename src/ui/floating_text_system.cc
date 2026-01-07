#include "ui/floating_text_system.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>

namespace {
constexpr float FLOATING_TEXT_LIFETIME = 0.8f;
constexpr float FLOATING_TEXT_SPEED = 24.0f;
} // namespace

FloatingTextSystem::FloatingTextSystem(EventBus& eventBus) : eventBus(eventBus) {}

void FloatingTextSystem::update(float dt) {
  for (auto& text : floatingTexts) {
    text.lifetime -= dt;
    text.position.y -= FLOATING_TEXT_SPEED * dt;
  }
  floatingTexts.erase(
      std::remove_if(floatingTexts.begin(), floatingTexts.end(),
                     [](const FloatingText& text) { return text.lifetime <= 0.0f; }),
      floatingTexts.end());

  for (const FloatingTextEvent& event : eventBus.consumeFloatingTextEvents()) {
    floatingTexts.push_back(FloatingText{event.text, event.position, FLOATING_TEXT_LIFETIME});
  }

  for (const RegionEvent& event : eventBus.consumeRegionEvents()) {
    const char* action = event.transition == RegionTransition::Enter ? "Entered " : "Left ";
    floatingTexts.push_back(FloatingText{std::string(action) + event.regionName, event.position,
                                         FLOATING_TEXT_LIFETIME});
  }
}

void FloatingTextSystem::render(SDL_Renderer* renderer, const Position& cameraPosition,
                                TTF_Font* font) {
  SDL_Color textColor = {255, 220, 220, 255};
  for (const FloatingText& text : floatingTexts) {
    SDL_Surface* surface =
        TTF_RenderText_Solid(font, text.text.c_str(), text.text.length(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect textRect = {text.position.x - cameraPosition.x, text.position.y - cameraPosition.y,
                          static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_RenderTexture(renderer, texture, nullptr, &textRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
  }
}
