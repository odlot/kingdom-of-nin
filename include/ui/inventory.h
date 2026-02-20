#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>

#include "ecs/component/class_component.h"
#include "ecs/component/equipment_component.h"
#include "ecs/component/inventory_component.h"
#include "ecs/component/level_component.h"
#include "items/item_database.h"

class Inventory {
public:
  Inventory() = default;

  void handleInput(const bool* keyboardState, int mouseX, int mouseY, bool mousePressed,
                   InventoryComponent& inventory, EquipmentComponent& equipment,
                   const ItemDatabase& database, const LevelComponent& level,
                   const ClassComponent& characterClass);
  void render(SDL_Renderer* renderer, TTF_Font* font, const InventoryComponent& inventory,
              const EquipmentComponent& equipment, const ItemDatabase& database);
  bool isStatsVisible() const { return isStatsOpen; }

private:
  bool wasInventoryPressed = false;
  bool wasEquipmentPressed = false;
  bool wasMousePressed = false;
  bool isInventoryOpen = false;
  bool isEquipmentOpen = false;
  bool wasStatsPressed = false;
  bool isStatsOpen = false;
  int lastMouseX = 0;
  int lastMouseY = 0;
};
