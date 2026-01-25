#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

#include "ecs/component/inventory_component.h"
#include "ecs/component/shop_component.h"
#include "ecs/component/stats_component.h"
#include "items/item_database.h"

struct ShopPanelState {
  float shopScroll = 0.0f;
  float inventoryScroll = 0.0f;
  std::string notice;
  float noticeTimer = 0.0f;
};

class ShopPanel {
public:
  ShopPanel() = default;

  void update(float dt, ShopPanelState& state) const;

  void handleInput(float mouseX, float mouseY, float mouseWheelDelta, bool click,
                   int windowWidth, int windowHeight,
                   ShopPanelState& state, ShopComponent& shop, InventoryComponent& inventory,
                   StatsComponent& stats, const ItemDatabase& itemDatabase) const;

  void render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight,
              float mouseX, float mouseY, const std::string& npcName,
              const ShopPanelState& state,
              const ShopComponent& shop, const InventoryComponent& inventory,
              const StatsComponent& stats, const ItemDatabase& itemDatabase) const;
};
