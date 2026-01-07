#include "ui/inventory.h"

#include <array>
#include <cstring>

namespace {
constexpr float LINE_HEIGHT = 16.0f;
constexpr float TITLE_Y_OFFSET = 6.0f;
constexpr float LIST_Y_OFFSET = 28.0f;
constexpr int INVENTORY_COLS = 4;
constexpr int INVENTORY_ROWS = 4;
constexpr float SLOT_SIZE = 44.0f;
constexpr float SLOT_PADDING = 6.0f;
constexpr float PANEL_PADDING = 8.0f;
constexpr float PANEL_WIDTH =
    (INVENTORY_COLS * SLOT_SIZE) + ((INVENTORY_COLS + 1) * SLOT_PADDING) + (PANEL_PADDING * 2.0f);
constexpr float PANEL_HEIGHT = LIST_Y_OFFSET + (INVENTORY_ROWS * SLOT_SIZE) +
                               ((INVENTORY_ROWS + 1) * SLOT_PADDING) + PANEL_PADDING;

const char* itemSlotName(ItemSlot slot) {
  switch (slot) {
  case ItemSlot::Weapon:
    return "Weapon";
  case ItemSlot::Shield:
    return "Shield";
  case ItemSlot::Shoulders:
    return "Shoulders";
  case ItemSlot::Chest:
    return "Chest";
  case ItemSlot::Pants:
    return "Pants";
  case ItemSlot::Boots:
    return "Boots";
  case ItemSlot::Cape:
    return "Cape";
  }
  return "Unknown";
}

bool pointInRect(int x, int y, const SDL_FRect& rect) {
  return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

SDL_FRect inventoryPanelRect() {
  return SDL_FRect{16.0f, 48.0f, PANEL_WIDTH, PANEL_HEIGHT};
}

SDL_FRect equipmentPanelRect() {
  SDL_FRect inv = inventoryPanelRect();
  return SDL_FRect{inv.x + inv.w + 16.0f, inv.y, inv.w, inv.h};
}

SDL_FRect inventoryGridOrigin(const SDL_FRect& panel) {
  return SDL_FRect{panel.x + SLOT_PADDING + PANEL_PADDING, panel.y + LIST_Y_OFFSET,
                   panel.w - (PANEL_PADDING * 2.0f), panel.h - LIST_Y_OFFSET - PANEL_PADDING};
}

std::optional<std::size_t> slotIndexFromPoint(int x, int y, const SDL_FRect& panel) {
  SDL_FRect grid = inventoryGridOrigin(panel);
  if (x < grid.x || y < grid.y) {
    return std::nullopt;
  }
  const float relX = x - grid.x;
  const float relY = y - grid.y;
  const int col = static_cast<int>(relX / (SLOT_SIZE + SLOT_PADDING));
  const int row = static_cast<int>(relY / (SLOT_SIZE + SLOT_PADDING));
  if (col < 0 || col >= INVENTORY_COLS || row < 0 || row >= INVENTORY_ROWS) {
    return std::nullopt;
  }
  const float cellX = col * (SLOT_SIZE + SLOT_PADDING);
  const float cellY = row * (SLOT_SIZE + SLOT_PADDING);
  if (relX > cellX + SLOT_SIZE || relY > cellY + SLOT_SIZE) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(row * INVENTORY_COLS + col);
}

SDL_FRect slotRect(std::size_t index, const SDL_FRect& panel) {
  const std::size_t col = index % INVENTORY_COLS;
  const std::size_t row = index / INVENTORY_COLS;
  SDL_FRect grid = inventoryGridOrigin(panel);
  return SDL_FRect{grid.x + (col * (SLOT_SIZE + SLOT_PADDING)),
                   grid.y + (row * (SLOT_SIZE + SLOT_PADDING)), SLOT_SIZE, SLOT_SIZE};
}

struct SlotBox {
  ItemSlot slot;
  SDL_FRect rect;
};

std::vector<SlotBox> equipmentSlotBoxes(const SDL_FRect& panel) {
  SDL_FRect grid = inventoryGridOrigin(panel);
  const float offsetX = grid.x;
  const float offsetY = grid.y;
  auto slotAt = [&](int col, int row) {
    return SDL_FRect{offsetX + (col * (SLOT_SIZE + SLOT_PADDING)),
                     offsetY + (row * (SLOT_SIZE + SLOT_PADDING)), SLOT_SIZE, SLOT_SIZE};
  };
  return {
      {ItemSlot::Shoulders, slotAt(1, 0)}, {ItemSlot::Shield, slotAt(0, 1)},
      {ItemSlot::Chest, slotAt(1, 1)},     {ItemSlot::Weapon, slotAt(2, 1)},
      {ItemSlot::Cape, slotAt(0, 2)},      {ItemSlot::Pants, slotAt(1, 2)},
      {ItemSlot::Boots, slotAt(2, 2)},
  };
}
} // namespace

void Inventory::handleInput(const bool* keyboardState, int mouseX, int mouseY, bool mousePressed,
                            InventoryComponent& inventory, EquipmentComponent& equipment,
                            const ItemDatabase& database) {
  lastMouseX = mouseX;
  lastMouseY = mouseY;

  const bool inventoryPressed = keyboardState[SDL_SCANCODE_I];
  const bool equipmentPressed = keyboardState[SDL_SCANCODE_E];
  const bool statsPressed = keyboardState[SDL_SCANCODE_S];
  if (inventoryPressed && !wasInventoryPressed) {
    isInventoryOpen = !isInventoryOpen;
  }
  if (equipmentPressed && !wasEquipmentPressed) {
    isEquipmentOpen = !isEquipmentOpen;
  }
  if (statsPressed && !wasStatsPressed) {
    isStatsOpen = !isStatsOpen;
  }
  wasInventoryPressed = inventoryPressed;
  wasEquipmentPressed = equipmentPressed;
  wasStatsPressed = statsPressed;

  const bool click = mousePressed && !wasMousePressed;
  wasMousePressed = mousePressed;

  if (!click) {
    return;
  }

  SDL_FRect panel = inventoryPanelRect();
  SDL_FRect equipPanel = equipmentPanelRect();

  if (isInventoryOpen && pointInRect(mouseX, mouseY, panel)) {
    std::optional<std::size_t> index = slotIndexFromPoint(mouseX, mouseY, panel);
    if (index.has_value() && *index < inventory.items.size()) {
      const ItemInstance item = inventory.items[*index];
      const ItemDef* def = database.getItem(item.itemId);
      if (def) {
        std::optional<ItemInstance> removed = inventory.takeItemAt(*index);
        if (!removed.has_value()) {
          return;
        }
        auto equippedIt = equipment.equipped.find(def->slot);
        if (equippedIt != equipment.equipped.end()) {
          if (!inventory.addItem(equippedIt->second)) {
            inventory.addItem(*removed);
            return;
          }
          equipment.equipped.erase(equippedIt);
        }
        equipment.equipped[def->slot] = *removed;
      }
    }
    return;
  }

  if (isEquipmentOpen && pointInRect(mouseX, mouseY, equipPanel)) {
    for (const SlotBox& box : equipmentSlotBoxes(equipPanel)) {
      if (!pointInRect(mouseX, mouseY, box.rect)) {
        continue;
      }
      auto it = equipment.equipped.find(box.slot);
      if (it != equipment.equipped.end()) {
        if (inventory.addItem(it->second)) {
          equipment.equipped.erase(it);
        }
      }
      break;
    }
  }
}

void Inventory::render(SDL_Renderer* renderer, TTF_Font* font, const InventoryComponent& inventory,
                       const EquipmentComponent& equipment, const ItemDatabase& database) {
  SDL_Color titleColor = {255, 255, 255, 255};
  SDL_FRect panel = inventoryPanelRect();
  SDL_FRect equipPanel = equipmentPanelRect();
  const SDL_FRect grid = inventoryGridOrigin(panel);
  const SDL_FRect equipGrid = inventoryGridOrigin(equipPanel);
  const bool mouseInPanel = pointInRect(lastMouseX, lastMouseY, panel);
  const std::optional<std::size_t> hoverIndex =
      (mouseInPanel && isInventoryOpen) ? slotIndexFromPoint(lastMouseX, lastMouseY, panel)
                                        : std::nullopt;
  const SlotBox* hoverEquip = nullptr;
  const bool mouseInEquip = pointInRect(lastMouseX, lastMouseY, equipPanel);
  if (mouseInEquip && isEquipmentOpen) {
    for (const SlotBox& box : equipmentSlotBoxes(equipPanel)) {
      if (pointInRect(lastMouseX, lastMouseY, box.rect)) {
        hoverEquip = &box;
        break;
      }
    }
  }

  if (isInventoryOpen) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
    SDL_RenderRect(renderer, &panel);

    SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Inventory", 9, titleColor);
    SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_FRect titleRect = {panel.x + 8.0f, panel.y + TITLE_Y_OFFSET,
                           static_cast<float>(titleSurface->w),
                           static_cast<float>(titleSurface->h)};
    SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);
    SDL_DestroySurface(titleSurface);
    SDL_DestroyTexture(titleTexture);

    for (std::size_t i = 0; i < InventoryComponent::kMaxSlots; ++i) {
      SDL_FRect slot = slotRect(i, panel);
      SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
      SDL_RenderFillRect(renderer, &slot);
      SDL_SetRenderDrawColor(renderer, 80, 80, 80, 220);
      SDL_RenderRect(renderer, &slot);

      if (i < inventory.items.size()) {
        const ItemDef* def = database.getItem(inventory.items[i].itemId);
        const char* name = def ? def->name.c_str() : "Item";
        SDL_Surface* lineSurface =
            TTF_RenderText_Solid(font, name, static_cast<int>(std::strlen(name)), titleColor);
        SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
        SDL_FRect lineRect = {slot.x + 4.0f, slot.y + 4.0f, static_cast<float>(lineSurface->w),
                              static_cast<float>(lineSurface->h)};
        SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
        SDL_DestroySurface(lineSurface);
        SDL_DestroyTexture(lineTexture);
      }
    }
  }

  if (isEquipmentOpen) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &equipPanel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
    SDL_RenderRect(renderer, &equipPanel);

    SDL_Surface* equipTitleSurface = TTF_RenderText_Solid(font, "Equipment", 9, titleColor);
    SDL_Texture* equipTitleTexture = SDL_CreateTextureFromSurface(renderer, equipTitleSurface);
    SDL_FRect equipTitleRect = {equipPanel.x + 8.0f, equipPanel.y + TITLE_Y_OFFSET,
                                static_cast<float>(equipTitleSurface->w),
                                static_cast<float>(equipTitleSurface->h)};
    SDL_RenderTexture(renderer, equipTitleTexture, nullptr, &equipTitleRect);
    SDL_DestroySurface(equipTitleSurface);
    SDL_DestroyTexture(equipTitleTexture);

    for (const SlotBox& box : equipmentSlotBoxes(equipPanel)) {
      SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
      SDL_RenderFillRect(renderer, &box.rect);
      SDL_SetRenderDrawColor(renderer, 80, 80, 80, 220);
      SDL_RenderRect(renderer, &box.rect);

      const char* slotLabel = itemSlotName(box.slot);
      SDL_Surface* labelSurface = TTF_RenderText_Solid(
          font, slotLabel, static_cast<int>(std::strlen(slotLabel)), titleColor);
      SDL_Texture* labelTexture = SDL_CreateTextureFromSurface(renderer, labelSurface);
      SDL_FRect labelRect = {box.rect.x + 4.0f, box.rect.y + 2.0f,
                             static_cast<float>(labelSurface->w),
                             static_cast<float>(labelSurface->h)};
      SDL_RenderTexture(renderer, labelTexture, nullptr, &labelRect);
      SDL_DestroySurface(labelSurface);
      SDL_DestroyTexture(labelTexture);

      auto it = equipment.equipped.find(box.slot);
      if (it != equipment.equipped.end()) {
        const ItemDef* def = database.getItem(it->second.itemId);
        const char* name = def ? def->name.c_str() : "Item";
        SDL_Surface* lineSurface =
            TTF_RenderText_Solid(font, name, static_cast<int>(std::strlen(name)), titleColor);
        SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
        SDL_FRect lineRect = {box.rect.x + 4.0f, box.rect.y + 18.0f,
                              static_cast<float>(lineSurface->w),
                              static_cast<float>(lineSurface->h)};
        SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
        SDL_DestroySurface(lineSurface);
        SDL_DestroyTexture(lineTexture);
      }
    }
  }

  if (hoverIndex.has_value() && *hoverIndex < inventory.items.size()) {
    const ItemDef* def = database.getItem(inventory.items[*hoverIndex].itemId);
    if (def) {
      std::string line1 = def->name;
      std::string line2 = std::string("Slot: ") + itemSlotName(def->slot);
      std::string line3 = "AP +" + std::to_string(def->stats.attackPower);
      std::string line4 = "Armor +" + std::to_string(def->stats.armor);

      SDL_Color textColor = {255, 255, 255, 255};
      const float tooltipX = grid.x + grid.w + 12.0f;
      const float tooltipY = grid.y;
      const float tooltipW = 180.0f;
      const float tooltipH = 72.0f;

      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, 10, 10, 10, 220);
      SDL_FRect tooltip = {tooltipX, tooltipY, tooltipW, tooltipH};
      SDL_RenderFillRect(renderer, &tooltip);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
      SDL_RenderRect(renderer, &tooltip);

      const std::array<std::string, 4> lines = {line1, line2, line3, line4};
      float textY = tooltipY + 6.0f;
      for (const std::string& line : lines) {
        SDL_Surface* surface =
            TTF_RenderText_Solid(font, line.c_str(), static_cast<int>(line.size()), textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FRect textRect = {tooltipX + 6.0f, textY, static_cast<float>(surface->w),
                              static_cast<float>(surface->h)};
        SDL_RenderTexture(renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
        textY += LINE_HEIGHT;
      }
    }
  }

  if (hoverEquip && isEquipmentOpen) {
    auto it = equipment.equipped.find(hoverEquip->slot);
    if (it != equipment.equipped.end()) {
      const ItemDef* def = database.getItem(it->second.itemId);
      if (def) {
        std::string line1 = def->name;
        std::string line2 = std::string("Slot: ") + itemSlotName(def->slot);
        std::string line3 = "AP +" + std::to_string(def->stats.attackPower);
        std::string line4 = "Armor +" + std::to_string(def->stats.armor);

        SDL_Color textColor = {255, 255, 255, 255};
        const float tooltipX = equipGrid.x + equipGrid.w + 12.0f;
        const float tooltipY = equipGrid.y + 84.0f;
        const float tooltipW = 180.0f;
        const float tooltipH = 72.0f;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 220);
        SDL_FRect tooltip = {tooltipX, tooltipY, tooltipW, tooltipH};
        SDL_RenderFillRect(renderer, &tooltip);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
        SDL_RenderRect(renderer, &tooltip);

        const std::array<std::string, 4> lines = {line1, line2, line3, line4};
        float textY = tooltipY + 6.0f;
        for (const std::string& line : lines) {
          SDL_Surface* surface =
              TTF_RenderText_Solid(font, line.c_str(), static_cast<int>(line.size()), textColor);
          SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
          SDL_FRect textRect = {tooltipX + 6.0f, textY, static_cast<float>(surface->w),
                                static_cast<float>(surface->h)};
          SDL_RenderTexture(renderer, texture, nullptr, &textRect);
          SDL_DestroySurface(surface);
          SDL_DestroyTexture(texture);
          textY += LINE_HEIGHT;
        }
      }
    }
  }
}
