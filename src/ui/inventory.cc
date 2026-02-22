#include "ui/inventory.h"

#include <string>
#include <vector>

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

SDL_Color rarityFrameColor(ItemRarity rarity) {
  switch (rarity) {
  case ItemRarity::Common:
    return SDL_Color{170, 170, 170, 255};
  case ItemRarity::Rare:
    return SDL_Color{90, 150, 255, 255};
  case ItemRarity::Epic:
    return SDL_Color{210, 120, 255, 255};
  }
  return SDL_Color{170, 170, 170, 255};
}

SDL_Color slotBaseColor(ItemSlot slot) {
  switch (slot) {
  case ItemSlot::Weapon:
    return SDL_Color{78, 56, 36, 255};
  case ItemSlot::Shield:
    return SDL_Color{58, 76, 102, 255};
  case ItemSlot::Shoulders:
    return SDL_Color{74, 70, 96, 255};
  case ItemSlot::Chest:
    return SDL_Color{88, 72, 62, 255};
  case ItemSlot::Pants:
    return SDL_Color{62, 74, 78, 255};
  case ItemSlot::Boots:
    return SDL_Color{70, 62, 56, 255};
  case ItemSlot::Cape:
    return SDL_Color{78, 58, 86, 255};
  }
  return SDL_Color{70, 70, 70, 255};
}

void drawGlyph(SDL_Renderer* renderer, const SDL_FRect& iconRect, ItemSlot slot,
               WeaponType weaponType, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

  switch (slot) {
  case ItemSlot::Weapon: {
    switch (weaponType) {
    case WeaponType::Bow: {
      SDL_RenderLine(renderer, iconRect.x + (iconRect.w * 0.28f), iconRect.y + 3.0f,
                     iconRect.x + (iconRect.w * 0.28f), iconRect.y + iconRect.h - 3.0f);
      SDL_RenderLine(renderer, iconRect.x + (iconRect.w * 0.70f), iconRect.y + 3.0f,
                     iconRect.x + (iconRect.w * 0.70f), iconRect.y + iconRect.h - 3.0f);
      SDL_RenderLine(renderer, iconRect.x + (iconRect.w * 0.28f), iconRect.y + (iconRect.h * 0.5f),
                     iconRect.x + (iconRect.w * 0.70f), iconRect.y + (iconRect.h * 0.5f));
      break;
    }
    case WeaponType::Wand: {
      SDL_FRect staff = {iconRect.x + (iconRect.w * 0.45f), iconRect.y + 6.0f, 4.0f,
                         iconRect.h - 12.0f};
      SDL_RenderFillRect(renderer, &staff);
      SDL_FRect orb = {iconRect.x + (iconRect.w * 0.39f), iconRect.y + 2.0f, 8.0f, 8.0f};
      SDL_RenderFillRect(renderer, &orb);
      break;
    }
    case WeaponType::Dagger: {
      SDL_FRect blade = {iconRect.x + (iconRect.w * 0.42f), iconRect.y + 5.0f, 6.0f,
                         iconRect.h * 0.46f};
      SDL_FRect handle = {iconRect.x + (iconRect.w * 0.36f), iconRect.y + (iconRect.h * 0.56f),
                          18.0f, 4.0f};
      SDL_RenderFillRect(renderer, &blade);
      SDL_RenderFillRect(renderer, &handle);
      break;
    }
    default: {
      SDL_FRect blade = {iconRect.x + (iconRect.w * 0.43f), iconRect.y + 4.0f, 5.0f,
                         iconRect.h * 0.56f};
      SDL_FRect guard = {iconRect.x + (iconRect.w * 0.30f), iconRect.y + (iconRect.h * 0.54f),
                         16.0f, 4.0f};
      SDL_FRect grip = {iconRect.x + (iconRect.w * 0.45f), iconRect.y + (iconRect.h * 0.64f), 3.0f,
                        iconRect.h * 0.22f};
      SDL_RenderFillRect(renderer, &blade);
      SDL_RenderFillRect(renderer, &guard);
      SDL_RenderFillRect(renderer, &grip);
      break;
    }
    }
    break;
  }
  case ItemSlot::Shield: {
    SDL_FRect body = {iconRect.x + 6.0f, iconRect.y + 4.0f, iconRect.w - 12.0f, iconRect.h - 10.0f};
    SDL_RenderFillRect(renderer, &body);
    SDL_SetRenderDrawColor(renderer, 22, 24, 28, 255);
    SDL_RenderLine(renderer, body.x + (body.w * 0.5f), body.y + 2.0f, body.x + (body.w * 0.5f),
                   body.y + body.h - 2.0f);
    SDL_RenderLine(renderer, body.x + 2.0f, body.y + (body.h * 0.5f), body.x + body.w - 2.0f,
                   body.y + (body.h * 0.5f));
    break;
  }
  case ItemSlot::Shoulders: {
    SDL_FRect left = {iconRect.x + 4.0f, iconRect.y + 7.0f, 10.0f, 8.0f};
    SDL_FRect right = {iconRect.x + iconRect.w - 14.0f, iconRect.y + 7.0f, 10.0f, 8.0f};
    SDL_FRect neck = {iconRect.x + (iconRect.w * 0.42f), iconRect.y + 10.0f, 6.0f, 12.0f};
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    SDL_RenderFillRect(renderer, &neck);
    break;
  }
  case ItemSlot::Chest: {
    SDL_FRect torso = {iconRect.x + 7.0f, iconRect.y + 6.0f, iconRect.w - 14.0f,
                       iconRect.h - 12.0f};
    SDL_RenderFillRect(renderer, &torso);
    break;
  }
  case ItemSlot::Pants: {
    SDL_FRect left = {iconRect.x + 8.0f, iconRect.y + 6.0f, 8.0f, iconRect.h - 10.0f};
    SDL_FRect right = {iconRect.x + iconRect.w - 16.0f, iconRect.y + 6.0f, 8.0f,
                       iconRect.h - 10.0f};
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    break;
  }
  case ItemSlot::Boots: {
    SDL_FRect left = {iconRect.x + 7.0f, iconRect.y + iconRect.h - 12.0f, 10.0f, 8.0f};
    SDL_FRect right = {iconRect.x + iconRect.w - 17.0f, iconRect.y + iconRect.h - 12.0f, 10.0f,
                       8.0f};
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    break;
  }
  case ItemSlot::Cape: {
    SDL_FRect cape = {iconRect.x + 9.0f, iconRect.y + 5.0f, iconRect.w - 18.0f, iconRect.h - 8.0f};
    SDL_RenderFillRect(renderer, &cape);
    break;
  }
  }
}

void renderItemIcon(SDL_Renderer* renderer, const SDL_FRect& slotRect, const ItemDef& def,
                    bool highlighted) {
  const SDL_Color base = slotBaseColor(def.slot);
  const SDL_Color frame = rarityFrameColor(def.rarity);
  const SDL_Color iconColor =
      highlighted ? SDL_Color{245, 245, 245, 255} : SDL_Color{230, 230, 230, 255};

  SDL_SetRenderDrawColor(renderer, base.r, base.g, base.b, 255);
  SDL_FRect inner = {slotRect.x + 3.0f, slotRect.y + 3.0f, slotRect.w - 6.0f, slotRect.h - 6.0f};
  SDL_RenderFillRect(renderer, &inner);

  SDL_SetRenderDrawColor(renderer, frame.r, frame.g, frame.b, 255);
  SDL_FRect frameRect = {slotRect.x + 1.0f, slotRect.y + 1.0f, slotRect.w - 2.0f,
                         slotRect.h - 2.0f};
  SDL_RenderRect(renderer, &frameRect);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
  SDL_FRect shadow = {inner.x + 1.0f, inner.y + 1.0f, inner.w, inner.h};
  SDL_RenderRect(renderer, &shadow);

  SDL_FRect glyphRect = {inner.x + 4.0f, inner.y + 4.0f, inner.w - 8.0f, inner.h - 8.0f};
  drawGlyph(renderer, glyphRect, def.slot, def.weaponType, iconColor);
}

void renderEmptySlotHint(SDL_Renderer* renderer, const SDL_FRect& slotRect, ItemSlot slot) {
  SDL_SetRenderDrawColor(renderer, 90, 90, 90, 150);
  SDL_FRect iconRect = {slotRect.x + 8.0f, slotRect.y + 8.0f, slotRect.w - 16.0f,
                        slotRect.h - 16.0f};
  drawGlyph(renderer, iconRect, slot, WeaponType::None, SDL_Color{90, 90, 90, 150});
}

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

const char* className(CharacterClass characterClass) {
  switch (characterClass) {
  case CharacterClass::Warrior:
    return "Warrior";
  case CharacterClass::Mage:
    return "Mage";
  case CharacterClass::Archer:
    return "Archer";
  case CharacterClass::Rogue:
    return "Rogue";
  case CharacterClass::Any:
    break;
  }
  return "Any";
}

std::string allowedClassesLabel(const ItemDef& def) {
  if (def.allowedClasses.empty() || def.allowedClasses.count(CharacterClass::Any) > 0) {
    return "All";
  }
  std::string label;
  for (CharacterClass entry : def.allowedClasses) {
    if (!label.empty()) {
      label += "/";
    }
    label += className(entry);
  }
  return label.empty() ? "All" : label;
}

std::vector<std::string> buildTooltipLines(const ItemDef& def) {
  std::vector<std::string> lines;
  const PrimaryStatBonuses& primary = primaryStatsForItem(def);
  lines.push_back(def.name);
  lines.push_back(std::string("Rarity: ") + itemRarityName(def.rarity));
  lines.push_back(std::string("Req L") + std::to_string(def.requiredLevel) + "  " +
                  allowedClassesLabel(def));
  lines.push_back(std::string("Slot: ") + itemSlotName(def.slot));
  lines.push_back("Armor +" + std::to_string(armorForItem(def)));
  if (primary.strength > 0) {
    lines.push_back("STR +" + std::to_string(primary.strength));
  }
  if (primary.dexterity > 0) {
    lines.push_back("DEX +" + std::to_string(primary.dexterity));
  }
  if (primary.intellect > 0) {
    lines.push_back("INT +" + std::to_string(primary.intellect));
  }
  if (primary.luck > 0) {
    lines.push_back("LUK +" + std::to_string(primary.luck));
  }
  return lines;
}

void renderTooltip(SDL_Renderer* renderer, TTF_Font* font, const std::vector<std::string>& lines,
                   float tooltipX, float tooltipY) {
  if (lines.empty()) {
    return;
  }
  SDL_Color textColor = {255, 255, 255, 255};
  const float tooltipW = 220.0f;
  const float tooltipH = 8.0f + (static_cast<float>(lines.size()) * LINE_HEIGHT) + 6.0f;

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 10, 10, 10, 220);
  SDL_FRect tooltip = {tooltipX, tooltipY, tooltipW, tooltipH};
  SDL_RenderFillRect(renderer, &tooltip);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
  SDL_RenderRect(renderer, &tooltip);

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
                            const ItemDatabase& database, const LevelComponent& level,
                            const ClassComponent& characterClass) {
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
        if (!meetsEquipRequirements(*def, level.level, characterClass.characterClass)) {
          return;
        }
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
        if (def) {
          renderItemIcon(renderer, slot, *def, hoverIndex.has_value() && *hoverIndex == i);
        }
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

      auto it = equipment.equipped.find(box.slot);
      if (it != equipment.equipped.end()) {
        const ItemDef* def = database.getItem(it->second.itemId);
        if (def) {
          renderItemIcon(renderer, box.rect, *def, hoverEquip && hoverEquip->slot == box.slot);
        }
      } else {
        renderEmptySlotHint(renderer, box.rect, box.slot);
      }
    }
  }

  if (hoverIndex.has_value() && *hoverIndex < inventory.items.size()) {
    const ItemDef* def = database.getItem(inventory.items[*hoverIndex].itemId);
    if (def) {
      const float tooltipX = grid.x + grid.w + 12.0f;
      const float tooltipY = grid.y;
      renderTooltip(renderer, font, buildTooltipLines(*def), tooltipX, tooltipY);
    }
  }

  if (hoverEquip && isEquipmentOpen) {
    auto it = equipment.equipped.find(hoverEquip->slot);
    if (it != equipment.equipped.end()) {
      const ItemDef* def = database.getItem(it->second.itemId);
      if (def) {
        const float tooltipX = equipGrid.x + equipGrid.w + 12.0f;
        const float tooltipY = equipGrid.y + 84.0f;
        renderTooltip(renderer, font, buildTooltipLines(*def), tooltipX, tooltipY);
      }
    }
  }
}
