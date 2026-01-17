#include "ui/skill_bar.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

namespace {
constexpr float SLOT_SIZE = 44.0f;
constexpr float SLOT_PADDING = 6.0f;
constexpr float PANEL_PADDING = 8.0f;
constexpr float PANEL_HEIGHT = SLOT_SIZE + (PANEL_PADDING * 2.0f);

SDL_FRect panelRect(int windowWidth, int windowHeight) {
  const float totalWidth = (SkillBarComponent::kSlotCount * SLOT_SIZE) +
                           ((SkillBarComponent::kSlotCount - 1) * SLOT_PADDING) +
                           (PANEL_PADDING * 2.0f);
  return SDL_FRect{(windowWidth - totalWidth) / 2.0f, windowHeight - PANEL_HEIGHT - 12.0f,
                   totalWidth, PANEL_HEIGHT};
}

SDL_FRect slotRect(std::size_t index, const SDL_FRect& panel) {
  const float x = panel.x + PANEL_PADDING + (index * (SLOT_SIZE + SLOT_PADDING));
  const float y = panel.y + PANEL_PADDING;
  return SDL_FRect{x, y, SLOT_SIZE, SLOT_SIZE};
}
} // namespace

void SkillBar::render(SDL_Renderer* renderer, TTF_Font* font, const SkillBarComponent& skills,
                      const SkillTreeComponent& tree, const SkillDatabase& database,
                      int windowWidth, int windowHeight) {
  SDL_FRect panel = panelRect(windowWidth, windowHeight);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 12, 12, 12, 200);
  SDL_RenderFillRect(renderer, &panel);

  for (std::size_t i = 0; i < skills.slots.size(); ++i) {
    SDL_FRect slot = slotRect(i, panel);
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &slot);
    SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);
    SDL_RenderRect(renderer, &slot);

    const SkillSlot& skillSlot = skills.slots[i];
    const SkillDef* def = (skillSlot.skillId > 0) ? database.getSkill(skillSlot.skillId) : nullptr;
    const bool unlocked = def && tree.unlockedSkills.count(def->id) > 0;
    if (def) {
      SDL_SetRenderDrawColor(renderer, 30, 90, 140, 255);
      SDL_FRect iconRect = {slot.x + 4.0f, slot.y + 4.0f, slot.w - 8.0f, slot.h - 8.0f};
      SDL_RenderFillRect(renderer, &iconRect);
    }

    const int slotNumber = static_cast<int>(i + 1);
    const std::string numberText = std::to_string(slotNumber);
    SDL_Color textColor = {230, 230, 230, 255};
    SDL_Surface* surface =
        TTF_RenderText_Solid(font, numberText.c_str(), numberText.length(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect textRect = {slot.x + 4.0f, slot.y + slot.h - surface->h - 4.0f,
                          static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_RenderTexture(renderer, texture, nullptr, &textRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);

    if (def && !unlocked) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
      SDL_RenderFillRect(renderer, &slot);
      SDL_SetRenderDrawColor(renderer, 200, 60, 60, 220);
      SDL_RenderRect(renderer, &slot);
    } else if (def && skillSlot.cooldownRemaining > 0.0f && def->cooldown > 0.0f) {
      const float ratio = std::clamp(skillSlot.cooldownRemaining / def->cooldown, 0.0f, 1.0f);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
      SDL_FRect cooldownRect = {slot.x, slot.y + (slot.h * (1.0f - ratio)), slot.w, slot.h * ratio};
      SDL_RenderFillRect(renderer, &cooldownRect);

      const int seconds = static_cast<int>(std::ceil(skillSlot.cooldownRemaining));
      const std::string cdText = std::to_string(seconds);
      SDL_Color cdColor = {255, 255, 255, 255};
      SDL_Surface* cdSurface = TTF_RenderText_Solid(font, cdText.c_str(), cdText.length(), cdColor);
      SDL_Texture* cdTexture = SDL_CreateTextureFromSurface(renderer, cdSurface);
      SDL_FRect cdRect = {slot.x + (slot.w - cdSurface->w) / 2.0f,
                          slot.y + (slot.h - cdSurface->h) / 2.0f, static_cast<float>(cdSurface->w),
                          static_cast<float>(cdSurface->h)};
      SDL_RenderTexture(renderer, cdTexture, nullptr, &cdRect);
      SDL_DestroySurface(cdSurface);
      SDL_DestroyTexture(cdTexture);
    }
  }
}
