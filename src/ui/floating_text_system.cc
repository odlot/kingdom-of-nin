#include "ui/floating_text_system.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>

namespace {
constexpr float FLOATING_TEXT_LIFETIME = 0.8f;
constexpr float FLOATING_TEXT_SPEED = 24.0f;
constexpr float CRIT_BASE_SCALE = 1.15f;
constexpr float CRIT_POP_EXTRA_SCALE = 0.2f;
constexpr float CRIT_POP_DURATION = 0.1f;

struct FloatingTextStyle {
  SDL_Color color;
  float scale = 1.0f;
  bool outline = true;
};

FloatingTextStyle styleForKind(FloatingTextKind kind) {
  switch (kind) {
  case FloatingTextKind::Damage:
    return FloatingTextStyle{SDL_Color{235, 90, 70, 255}, 1.0f, true};
  case FloatingTextKind::CritDamage:
    return FloatingTextStyle{SDL_Color{255, 210, 100, 255}, CRIT_BASE_SCALE, true};
  case FloatingTextKind::Heal:
    return FloatingTextStyle{SDL_Color{120, 230, 140, 255}, 1.0f, true};
  case FloatingTextKind::Info:
    return FloatingTextStyle{SDL_Color{235, 235, 235, 255}, 1.0f, true};
  }
  return FloatingTextStyle{SDL_Color{235, 235, 235, 255}, 1.0f, true};
}
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
    floatingTexts.push_back(
        FloatingText{event.text, event.position, FLOATING_TEXT_LIFETIME, event.kind});
  }

  for (const RegionEvent& event : eventBus.consumeRegionEvents()) {
    const char* action = event.transition == RegionTransition::Enter ? "Entered " : "Left ";
    floatingTexts.push_back(FloatingText{std::string(action) + event.regionName, event.position,
                                         FLOATING_TEXT_LIFETIME, FloatingTextKind::Info});
  }
}

void FloatingTextSystem::render(SDL_Renderer* renderer, const Position& cameraPosition,
                                TTF_Font* font) {
  for (const FloatingText& text : floatingTexts) {
    const float lifeRatio = std::clamp(text.lifetime / FLOATING_TEXT_LIFETIME, 0.0f, 1.0f);
    const Uint8 alpha = static_cast<Uint8>(255.0f * lifeRatio);
    FloatingTextStyle style = styleForKind(text.kind);
    float scale = style.scale;
    if (text.kind == FloatingTextKind::CritDamage) {
      const float age = FLOATING_TEXT_LIFETIME - text.lifetime;
      if (age < CRIT_POP_DURATION) {
        const float popT = 1.0f - (age / CRIT_POP_DURATION);
        scale += CRIT_POP_EXTRA_SCALE * popT;
      }
    }

    SDL_Color textColor = style.color;
    textColor.a = 255;
    SDL_Surface* surface =
        TTF_RenderText_Solid(font, text.text.c_str(), text.text.length(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(texture, alpha);
    const float baseX = text.position.x - cameraPosition.x;
    const float baseY = text.position.y - cameraPosition.y;
    SDL_FRect textRect = {baseX, baseY, static_cast<float>(surface->w) * scale,
                          static_cast<float>(surface->h) * scale};

    if (style.outline) {
      SDL_Color outlineColor = {0, 0, 0, 255};
      SDL_Surface* outlineSurface =
          TTF_RenderText_Solid(font, text.text.c_str(), text.text.length(), outlineColor);
      SDL_Texture* outlineTexture = SDL_CreateTextureFromSurface(renderer, outlineSurface);
      SDL_SetTextureBlendMode(outlineTexture, SDL_BLENDMODE_BLEND);
      SDL_SetTextureAlphaMod(outlineTexture, alpha);
      const float offset = 1.0f;
      const SDL_FPoint offsets[] = {{-offset, 0.0f}, {offset, 0.0f}, {0.0f, -offset},
                                    {0.0f, offset}};
      for (const SDL_FPoint& delta : offsets) {
        SDL_FRect outlineRect = {textRect.x + delta.x, textRect.y + delta.y, textRect.w,
                                 textRect.h};
        SDL_RenderTexture(renderer, outlineTexture, nullptr, &outlineRect);
      }
      SDL_DestroySurface(outlineSurface);
      SDL_DestroyTexture(outlineTexture);
    }

    SDL_RenderTexture(renderer, texture, nullptr, &textRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
  }
}
