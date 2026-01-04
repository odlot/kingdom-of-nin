#pragma once

#include <string>
#include <vector>

#include "SDL3/SDL.h"
#include "ecs/position.h"
#include "events/event_bus.h"
#include <SDL3_ttf/SDL_ttf.h>

class FloatingTextSystem {
public:
  struct FloatingText {
    std::string text;
    Position position;
    float lifetime = 0.0f;
  };

  explicit FloatingTextSystem(EventBus& eventBus);

  void update(float dt);
  void render(SDL_Renderer* renderer, const Position& cameraPosition, TTF_Font* font);

private:
  EventBus& eventBus;
  std::vector<FloatingText> floatingTexts;
};
