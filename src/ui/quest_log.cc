#include "ui/quest_log.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {
constexpr float PANEL_WIDTH = 320.0f;
constexpr float PANEL_HEIGHT = 260.0f;
constexpr float PANEL_PADDING = 10.0f;
constexpr float LINE_HEIGHT = 16.0f;
constexpr float TITLE_OFFSET = 6.0f;

SDL_FRect panelRect(int windowWidth, int windowHeight) {
  const float x = 12.0f;
  const float y = 72.0f;
  return SDL_FRect{x, y, PANEL_WIDTH, PANEL_HEIGHT};
}

std::string objectiveText(const QuestObjectiveProgress& objective, const ItemDatabase& items) {
  switch (objective.def.type) {
  case QuestObjectiveType::KillMobType: {
    std::string mobName = "Mob";
    switch (objective.def.mobType) {
    case MobType::Goblin:
      mobName = "Goblin";
      break;
    case MobType::GoblinArcher:
      mobName = "Goblin Archer";
      break;
    case MobType::GoblinBrute:
      mobName = "Goblin Brute";
      break;
    }
    return "Defeat " + mobName + " (" + std::to_string(objective.currentCount) + "/" +
           std::to_string(objective.def.requiredCount) + ")";
  }
  case QuestObjectiveType::CollectItem: {
    const ItemDef* def = items.getItem(objective.def.itemId);
    const std::string name = def ? def->name : "Item";
    return "Collect " + name + " (" + std::to_string(objective.currentCount) + "/" +
           std::to_string(objective.def.requiredCount) + ")";
  }
  case QuestObjectiveType::EnterRegion: {
    return "Visit " + objective.def.regionName;
  }
  }
  return "Objective";
}
} // namespace

SDL_FRect QuestLog::panelRect(int windowWidth, int windowHeight) const {
  return ::panelRect(windowWidth, windowHeight);
}

void QuestLog::render(SDL_Renderer* renderer, TTF_Font* font,
                      const QuestLogComponent& questLog, const QuestDatabase& questDatabase,
                      const ItemDatabase& itemDatabase, int windowWidth, int windowHeight,
                      bool isVisible, float& scroll) const {
  if (!isVisible) {
    return;
  }

  SDL_FRect panel = panelRect(windowWidth, windowHeight);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 10, 10, 10, 200);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 160);
  SDL_RenderRect(renderer, &panel);

  SDL_Color titleColor = {255, 240, 210, 255};
  SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Quest Log", 9, titleColor);
  SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
  SDL_FRect titleRect = {panel.x + PANEL_PADDING, panel.y + TITLE_OFFSET,
                         static_cast<float>(titleSurface->w),
                         static_cast<float>(titleSurface->h)};
  SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);
  SDL_DestroySurface(titleSurface);
  SDL_DestroyTexture(titleTexture);

  float textY = panel.y + 28.0f;
  SDL_Color textColor = {235, 235, 235, 255};
  SDL_Color mutedColor = {160, 160, 160, 255};
  SDL_Color headerColor = {210, 210, 210, 255};
  struct LineEntry {
    std::string text;
    SDL_Color color;
    float indent = 0.0f;
    bool isHeader = false;
  };
  std::vector<LineEntry> lines;

  if (questLog.activeQuests.empty()) {
    SDL_Surface* emptySurface =
        TTF_RenderText_Solid(font, "No active quests.", 17, mutedColor);
    SDL_Texture* emptyTexture = SDL_CreateTextureFromSurface(renderer, emptySurface);
    SDL_FRect emptyRect = {panel.x + PANEL_PADDING, textY,
                           static_cast<float>(emptySurface->w),
                           static_cast<float>(emptySurface->h)};
    SDL_RenderTexture(renderer, emptyTexture, nullptr, &emptyRect);
    SDL_DestroySurface(emptySurface);
    SDL_DestroyTexture(emptyTexture);
    return;
  }

  auto pushCategory = [&](QuestCategory category, const std::string& name) {
    lines.push_back(LineEntry{name, headerColor, 0.0f, true});
    for (const QuestProgress& progress : questLog.activeQuests) {
      const QuestDef* def = questDatabase.getQuest(progress.questId);
      if (!def || def->category != category) {
        continue;
      }
      const std::string title =
          def->name + (progress.completed ? " (Complete)" : "");
      lines.push_back(LineEntry{title, textColor, 0.0f, false});
      for (const QuestObjectiveProgress& objective : progress.objectives) {
        lines.push_back(LineEntry{"- " + objectiveText(objective, itemDatabase), mutedColor, 6.0f,
                                  false});
      }
      lines.push_back(LineEntry{"", mutedColor, 0.0f, false});
    }
  };

  pushCategory(QuestCategory::Main, "Main");
  pushCategory(QuestCategory::Side, "Side");
  pushCategory(QuestCategory::Bounty, "Bounty");

  if (!lines.empty() && lines.back().text.empty()) {
    lines.pop_back();
  }

  const float contentHeight = static_cast<float>(lines.size()) * LINE_HEIGHT;
  const float viewHeight = panel.h - 40.0f;
  const float maxScroll = std::max(0.0f, contentHeight - viewHeight);
  scroll = std::clamp(scroll, 0.0f, maxScroll);
  const int startLine = static_cast<int>(std::floor(scroll / LINE_HEIGHT));
  const float lineOffset = std::fmod(scroll, LINE_HEIGHT);
  const int maxVisible = static_cast<int>(std::ceil(viewHeight / LINE_HEIGHT)) + 1;
  const int endLine = std::min(static_cast<int>(lines.size()), startLine + maxVisible);
  textY = panel.y + 28.0f - lineOffset;

  for (int i = startLine; i < endLine; ++i) {
    const LineEntry& entry = lines[i];
    if (!entry.text.empty()) {
      SDL_Surface* lineSurface =
          TTF_RenderText_Solid(font, entry.text.c_str(), entry.text.length(), entry.color);
      SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer, lineSurface);
      SDL_FRect lineRect = {panel.x + PANEL_PADDING + entry.indent, textY,
                            static_cast<float>(lineSurface->w),
                            static_cast<float>(lineSurface->h)};
      SDL_RenderTexture(renderer, lineTexture, nullptr, &lineRect);
      SDL_DestroySurface(lineSurface);
      SDL_DestroyTexture(lineTexture);
    }
    textY += LINE_HEIGHT;
  }

  if (maxScroll > 0.0f) {
    const float trackX = panel.x + panel.w - 8.0f;
    const float trackY = panel.y + 28.0f;
    const float trackH = viewHeight;
    const float thumbH = std::max(18.0f, (viewHeight / contentHeight) * trackH);
    const float scrollRatio = (maxScroll > 0.0f) ? (scroll / maxScroll) : 0.0f;
    const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 220);
    SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
    SDL_RenderFillRect(renderer, &trackRect);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 220);
    SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
    SDL_RenderFillRect(renderer, &thumbRect);
  }
}
