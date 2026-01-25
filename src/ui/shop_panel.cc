#include "ui/shop_panel.h"

#include <algorithm>

namespace {
constexpr float kRowHeight = 16.0f;
constexpr float kPanelWidth = 520.0f;
constexpr float kPanelHeight = 200.0f;
constexpr float kPanelPadding = 12.0f;
constexpr float kColumnGap = 12.0f;
constexpr float kNoticeDuration = 1.6f;
constexpr int kSellDivisor = 2;

struct ShopLayout {
  SDL_FRect panel;
  SDL_FRect shopRect;
  SDL_FRect inventoryRect;
};

ShopLayout shopLayout(int windowWidth, int windowHeight) {
  const float x = (windowWidth - kPanelWidth) / 2.0f;
  const float y = windowHeight - kPanelHeight - 120.0f;
  ShopLayout layout;
  layout.panel = SDL_FRect{x, y, kPanelWidth, kPanelHeight};
  const float listWidth = (kPanelWidth - (kColumnGap * 3.0f)) / 2.0f;
  layout.shopRect = SDL_FRect{x + kColumnGap, y + 36.0f, listWidth, kPanelHeight - 48.0f};
  layout.inventoryRect =
      SDL_FRect{x + (kColumnGap * 2.0f) + listWidth, y + 36.0f, listWidth, kPanelHeight - 48.0f};
  return layout;
}

bool pointInRect(float x, float y, const SDL_FRect& rect) {
  return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

int itemPrice(const ItemDef* def) {
  return def ? def->price : 0;
}

int visibleRows(const SDL_FRect& rect) {
  return std::max(1, static_cast<int>(std::floor(rect.h / kRowHeight)));
}
} // namespace

void ShopPanel::update(float dt, ShopPanelState& state) const {
  state.noticeTimer = std::max(0.0f, state.noticeTimer - dt);
}

void ShopPanel::handleInput(float mouseX, float mouseY, float mouseWheelDelta, bool click,
                            int windowWidth, int windowHeight, ShopPanelState& state,
                            ShopComponent& shop,
                            InventoryComponent& inventory, StatsComponent& stats,
                            const ItemDatabase& itemDatabase) const {
  const ShopLayout layout = shopLayout(windowWidth, windowHeight);
  const int shopVisibleRows = visibleRows(layout.shopRect);
  const int invVisibleRows = visibleRows(layout.inventoryRect);
  const int shopMaxScrollRows = std::max(0, static_cast<int>(shop.stock.size()) - shopVisibleRows);
  const int invMaxScrollRows =
      std::max(0, static_cast<int>(inventory.items.size()) - invVisibleRows);

  if (mouseWheelDelta != 0.0f) {
    if (pointInRect(mouseX, mouseY, layout.shopRect)) {
      state.shopScroll -= mouseWheelDelta * kRowHeight;
    } else if (pointInRect(mouseX, mouseY, layout.inventoryRect)) {
      state.inventoryScroll -= mouseWheelDelta * kRowHeight;
    } else {
      state.shopScroll -= mouseWheelDelta * kRowHeight;
      state.inventoryScroll -= mouseWheelDelta * kRowHeight;
    }
  }
  state.shopScroll = std::clamp(state.shopScroll, 0.0f,
                                static_cast<float>(shopMaxScrollRows) * kRowHeight);
  state.inventoryScroll = std::clamp(state.inventoryScroll, 0.0f,
                                     static_cast<float>(invMaxScrollRows) * kRowHeight);

  const int shopStartIndex = static_cast<int>(std::floor(state.shopScroll / kRowHeight));
  const int invStartIndex = static_cast<int>(std::floor(state.inventoryScroll / kRowHeight));

  if (click && pointInRect(mouseX, mouseY, layout.shopRect)) {
    const int row = static_cast<int>(std::floor((mouseY - layout.shopRect.y) / kRowHeight));
    const int index = shopStartIndex + row;
    if (row >= 0 && row < shopVisibleRows && index >= 0 &&
        index < static_cast<int>(shop.stock.size())) {
      const int itemId = shop.stock[index];
      const ItemDef* def = itemDatabase.getItem(itemId);
      const int price = itemPrice(def);
      if (def && price > 0) {
        if (stats.gold < price) {
          state.notice = "Not enough gold.";
          state.noticeTimer = kNoticeDuration;
        } else if (!inventory.addItem(ItemInstance{itemId})) {
          state.notice = "Inventory full.";
          state.noticeTimer = kNoticeDuration;
        } else {
          stats.gold -= price;
          state.notice = "Purchased " + def->name + ".";
          state.noticeTimer = kNoticeDuration;
        }
      }
    }
  }

  if (click && pointInRect(mouseX, mouseY, layout.inventoryRect)) {
    const int row = static_cast<int>(std::floor((mouseY - layout.inventoryRect.y) / kRowHeight));
    const int index = invStartIndex + row;
    if (row >= 0 && row < invVisibleRows && index >= 0 &&
        index < static_cast<int>(inventory.items.size())) {
      const ItemDef* def = itemDatabase.getItem(inventory.items[index].itemId);
      const int basePrice = itemPrice(def);
      const int sellPrice = basePrice > 0 ? std::max(1, basePrice / kSellDivisor) : 0;
      if (inventory.removeItemAt(static_cast<std::size_t>(index))) {
        stats.gold += sellPrice;
        if (def) {
          state.notice = "Sold " + def->name + ".";
        } else {
          state.notice = "Sold item.";
        }
        state.noticeTimer = kNoticeDuration;
      }
    }
  }
}

void ShopPanel::render(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight,
                       float mouseX, float mouseY, const std::string& npcName,
                       const ShopPanelState& state,
                       const ShopComponent& shop, const InventoryComponent& inventory,
                       const StatsComponent& stats, const ItemDatabase& itemDatabase) const {
  const ShopLayout layout = shopLayout(windowWidth, windowHeight);
  const int shopVisibleRows = visibleRows(layout.shopRect);
  const int invVisibleRows = visibleRows(layout.inventoryRect);
  const int shopMaxScrollRows = std::max(0, static_cast<int>(shop.stock.size()) - shopVisibleRows);
  const int invMaxScrollRows =
      std::max(0, static_cast<int>(inventory.items.size()) - invVisibleRows);
  const float shopScroll = std::clamp(state.shopScroll, 0.0f,
                                      static_cast<float>(shopMaxScrollRows) * kRowHeight);
  const float inventoryScroll = std::clamp(state.inventoryScroll, 0.0f,
                                           static_cast<float>(invMaxScrollRows) * kRowHeight);
  const int shopStartIndex = static_cast<int>(std::floor(shopScroll / kRowHeight));
  const int invStartIndex = static_cast<int>(std::floor(inventoryScroll / kRowHeight));

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 12, 12, 12, 220);
  SDL_RenderFillRect(renderer, &layout.panel);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 140);
  SDL_RenderRect(renderer, &layout.panel);

  SDL_Color titleColor = {255, 240, 210, 255};
  SDL_Color headerColor = {220, 220, 220, 255};
  SDL_Color textColor = {235, 235, 235, 255};
  SDL_Color hintColor = {180, 180, 180, 255};

  const std::string title = npcName + " - Shop";
  SDL_Surface* titleSurface =
      TTF_RenderText_Solid(font, title.c_str(), title.length(), titleColor);
  SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
  SDL_FRect titleRect = {layout.panel.x + 12.0f, layout.panel.y + 8.0f,
                         static_cast<float>(titleSurface->w),
                         static_cast<float>(titleSurface->h)};
  SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);
  SDL_DestroySurface(titleSurface);
  SDL_DestroyTexture(titleTexture);

  const std::string goldText = "Gold: " + std::to_string(stats.gold);
  SDL_Surface* goldSurface =
      TTF_RenderText_Solid(font, goldText.c_str(), goldText.length(), headerColor);
  SDL_Texture* goldTexture = SDL_CreateTextureFromSurface(renderer, goldSurface);
  SDL_FRect goldRect = {layout.panel.x + layout.panel.w - goldSurface->w - 12.0f,
                        layout.panel.y + 8.0f, static_cast<float>(goldSurface->w),
                        static_cast<float>(goldSurface->h)};
  SDL_RenderTexture(renderer, goldTexture, nullptr, &goldRect);
  SDL_DestroySurface(goldSurface);
  SDL_DestroyTexture(goldTexture);

  const std::string shopHeader = "Shop";
  SDL_Surface* shopSurface =
      TTF_RenderText_Solid(font, shopHeader.c_str(), shopHeader.length(), headerColor);
  SDL_Texture* shopTexture = SDL_CreateTextureFromSurface(renderer, shopSurface);
  SDL_FRect shopHeaderRect = {layout.shopRect.x, layout.shopRect.y - 18.0f,
                              static_cast<float>(shopSurface->w),
                              static_cast<float>(shopSurface->h)};
  SDL_RenderTexture(renderer, shopTexture, nullptr, &shopHeaderRect);
  SDL_DestroySurface(shopSurface);
  SDL_DestroyTexture(shopTexture);

  const std::string invHeader = "Inventory";
  SDL_Surface* invSurface =
      TTF_RenderText_Solid(font, invHeader.c_str(), invHeader.length(), headerColor);
  SDL_Texture* invTexture = SDL_CreateTextureFromSurface(renderer, invSurface);
  SDL_FRect invHeaderRect = {layout.inventoryRect.x, layout.inventoryRect.y - 18.0f,
                             static_cast<float>(invSurface->w),
                             static_cast<float>(invSurface->h)};
  SDL_RenderTexture(renderer, invTexture, nullptr, &invHeaderRect);
  SDL_DestroySurface(invSurface);
  SDL_DestroyTexture(invTexture);

  const int shopHoveredRow =
      pointInRect(mouseX, mouseY, layout.shopRect)
          ? static_cast<int>(std::floor((mouseY - layout.shopRect.y) / kRowHeight))
          : -1;
  const int invHoveredRow =
      pointInRect(mouseX, mouseY, layout.inventoryRect)
          ? static_cast<int>(std::floor((mouseY - layout.inventoryRect.y) / kRowHeight))
          : -1;

  for (int i = 0; i < shopVisibleRows; ++i) {
    const int index = shopStartIndex + i;
    if (index < 0 || index >= static_cast<int>(shop.stock.size())) {
      break;
    }
    const ItemDef* def = itemDatabase.getItem(shop.stock[index]);
    if (!def) {
      continue;
    }
    if (i == shopHoveredRow) {
      SDL_SetRenderDrawColor(renderer, 60, 60, 90, 140);
      SDL_FRect rowRect = {layout.shopRect.x, layout.shopRect.y + (i * kRowHeight),
                           layout.shopRect.w, kRowHeight};
      SDL_RenderFillRect(renderer, &rowRect);
    }
    const int price = itemPrice(def);
    const std::string line = def->name + " - " + std::to_string(price);
    SDL_Surface* lineSurface =
        TTF_RenderText_Solid(font, line.c_str(), line.length(), textColor);
    SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
    SDL_FRect lineRect = {layout.shopRect.x, layout.shopRect.y + (i * kRowHeight),
                          static_cast<float>(lineSurface->w),
                          static_cast<float>(lineSurface->h)};
    SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
    SDL_DestroySurface(lineSurface);
    SDL_DestroyTexture(lineTexture);
  }

  for (int i = 0; i < invVisibleRows; ++i) {
    const int index = invStartIndex + i;
    if (index < 0 || index >= static_cast<int>(inventory.items.size())) {
      break;
    }
    const ItemDef* def = itemDatabase.getItem(inventory.items[index].itemId);
    if (!def) {
      continue;
    }
    if (i == invHoveredRow) {
      SDL_SetRenderDrawColor(renderer, 70, 60, 40, 140);
      SDL_FRect rowRect = {layout.inventoryRect.x, layout.inventoryRect.y + (i * kRowHeight),
                           layout.inventoryRect.w, kRowHeight};
      SDL_RenderFillRect(renderer, &rowRect);
    }
    const int basePrice = itemPrice(def);
    const int sellPrice = basePrice > 0 ? std::max(1, basePrice / kSellDivisor) : 0;
    const std::string line = def->name + " - " + std::to_string(sellPrice);
    SDL_Surface* lineSurface =
        TTF_RenderText_Solid(font, line.c_str(), line.length(), textColor);
    SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
    SDL_FRect lineRect = {layout.inventoryRect.x, layout.inventoryRect.y + (i * kRowHeight),
                          static_cast<float>(lineSurface->w),
                          static_cast<float>(lineSurface->h)};
    SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
    SDL_DestroySurface(lineSurface);
    SDL_DestroyTexture(lineTexture);
  }

  const std::string hint = "Click item to buy/sell";
  SDL_Surface* hintSurface =
      TTF_RenderText_Solid(font, hint.c_str(), hint.length(), hintColor);
  SDL_Texture* hintTexture = SDL_CreateTextureFromSurface(renderer, hintSurface);
  SDL_FRect hintRect = {layout.panel.x + 12.0f, layout.panel.y + layout.panel.h - 18.0f,
                        static_cast<float>(hintSurface->w),
                        static_cast<float>(hintSurface->h)};
  SDL_RenderTexture(renderer, hintTexture, nullptr, &hintRect);
  SDL_DestroySurface(hintSurface);
  SDL_DestroyTexture(hintTexture);

  if (shopHoveredRow >= 0) {
    const int index = shopStartIndex + shopHoveredRow;
    const ItemDef* def =
        (index >= 0 && index < static_cast<int>(shop.stock.size()))
            ? itemDatabase.getItem(shop.stock[index])
            : nullptr;
    if (def) {
      const std::string tip = "Buy for " + std::to_string(itemPrice(def)) + " gold";
      SDL_Surface* tipSurface =
          TTF_RenderText_Solid(font, tip.c_str(), tip.length(), headerColor);
      SDL_Texture* tipTexture = SDL_CreateTextureFromSurface(renderer, tipSurface);
      SDL_FRect tipRect = {layout.shopRect.x,
                           layout.shopRect.y + (shopHoveredRow * kRowHeight) - 18.0f,
                           static_cast<float>(tipSurface->w),
                           static_cast<float>(tipSurface->h)};
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
      SDL_FRect tipBg = {tipRect.x - 4.0f, tipRect.y - 2.0f, tipRect.w + 8.0f,
                         tipRect.h + 4.0f};
      SDL_RenderFillRect(renderer, &tipBg);
      SDL_RenderTexture(renderer, tipTexture, nullptr, &tipRect);
      SDL_DestroySurface(tipSurface);
      SDL_DestroyTexture(tipTexture);
    }
  }

  if (invHoveredRow >= 0) {
    const int index = invStartIndex + invHoveredRow;
    const ItemDef* def = (index >= 0 && index < static_cast<int>(inventory.items.size()))
                             ? itemDatabase.getItem(inventory.items[index].itemId)
                             : nullptr;
    if (def) {
      const int basePrice = itemPrice(def);
      const int sellPrice = basePrice > 0 ? std::max(1, basePrice / kSellDivisor) : 0;
      const std::string tip = "Sell for " + std::to_string(sellPrice) + " gold";
      SDL_Surface* tipSurface =
          TTF_RenderText_Solid(font, tip.c_str(), tip.length(), headerColor);
      SDL_Texture* tipTexture = SDL_CreateTextureFromSurface(renderer, tipSurface);
      SDL_FRect tipRect = {layout.inventoryRect.x,
                           layout.inventoryRect.y + (invHoveredRow * kRowHeight) - 18.0f,
                           static_cast<float>(tipSurface->w),
                           static_cast<float>(tipSurface->h)};
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
      SDL_FRect tipBg = {tipRect.x - 4.0f, tipRect.y - 2.0f, tipRect.w + 8.0f,
                         tipRect.h + 4.0f};
      SDL_RenderFillRect(renderer, &tipBg);
      SDL_RenderTexture(renderer, tipTexture, nullptr, &tipRect);
      SDL_DestroySurface(tipSurface);
      SDL_DestroyTexture(tipTexture);
    }
  }

  if (shopMaxScrollRows > 0) {
    const float trackX = layout.shopRect.x + layout.shopRect.w - 6.0f;
    const float trackY = layout.shopRect.y;
    const float trackH = layout.shopRect.h;
    const float thumbH = std::max(18.0f, (static_cast<float>(shopVisibleRows) /
                                          static_cast<float>(shop.stock.size())) *
                                             trackH);
    const float scrollRatio =
        (shopMaxScrollRows > 0)
            ? (shopScroll / (static_cast<float>(shopMaxScrollRows) * kRowHeight))
            : 0.0f;
    const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
    SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
    SDL_RenderFillRect(renderer, &trackRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 220);
    SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
    SDL_RenderFillRect(renderer, &thumbRect);
  }

  if (invMaxScrollRows > 0) {
    const float trackX = layout.inventoryRect.x + layout.inventoryRect.w - 6.0f;
    const float trackY = layout.inventoryRect.y;
    const float trackH = layout.inventoryRect.h;
    const float thumbH = std::max(18.0f, (static_cast<float>(invVisibleRows) /
                                          static_cast<float>(inventory.items.size())) *
                                             trackH);
    const float scrollRatio =
        (invMaxScrollRows > 0)
            ? (inventoryScroll / (static_cast<float>(invMaxScrollRows) * kRowHeight))
            : 0.0f;
    const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
    SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
    SDL_RenderFillRect(renderer, &trackRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 220);
    SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
    SDL_RenderFillRect(renderer, &thumbRect);
  }

  if (state.noticeTimer > 0.0f && !state.notice.empty()) {
    SDL_Surface* noticeSurface =
        TTF_RenderText_Solid(font, state.notice.c_str(), state.notice.length(), headerColor);
    SDL_Texture* noticeTexture = SDL_CreateTextureFromSurface(renderer, noticeSurface);
    SDL_FRect noticeRect = {layout.panel.x + 12.0f, layout.panel.y - 20.0f,
                            static_cast<float>(noticeSurface->w),
                            static_cast<float>(noticeSurface->h)};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect noticeBg = {noticeRect.x - 6.0f, noticeRect.y - 4.0f, noticeRect.w + 12.0f,
                          noticeRect.h + 8.0f};
    SDL_RenderFillRect(renderer, &noticeBg);
    SDL_RenderTexture(renderer, noticeTexture, nullptr, &noticeRect);
    SDL_DestroySurface(noticeSurface);
    SDL_DestroyTexture(noticeTexture);
  }
}
