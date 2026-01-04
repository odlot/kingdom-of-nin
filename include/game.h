#pragma once

#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include <vector>

#include "camera.h"
#include "ecs/registry.h"
#include "events/event_bus.h"
#include "items/item_database.h"
#include "ui/floating_text_system.h"
#include "ui/minimap.h"
#include "world/map.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

class Game {
public:
  Game();
  ~Game();

  bool isRunning();
  void update(float dt);
  void interpolate(float alpha);
  void render();

private:
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  TTF_Font* font = nullptr;
  std::unique_ptr<Camera> camera;
  std::unique_ptr<Minimap> minimap;
  std::unique_ptr<ItemDatabase> itemDatabase;
  std::unique_ptr<EventBus> eventBus;
  std::unique_ptr<FloatingTextSystem> floatingTextSystem;
  bool running = true;
  std::unique_ptr<Registry> registry;
  std::unique_ptr<Map> map;
  int playerEntityId = -1;
  std::vector<int> mobEntityIds;
  float attackCooldownRemaining = 0.0f;
  bool wasAttackPressed = false;
};
