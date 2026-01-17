#include "ui/skill_tree.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace {
constexpr float PANEL_WIDTH = 360.0f;
constexpr float PANEL_HEIGHT = 300.0f;
constexpr float PANEL_PADDING = 10.0f;
constexpr float NODE_SIZE = 48.0f;
constexpr float NODE_PADDING = 16.0f;
constexpr float GRID_OFFSET_X = 40.0f;
constexpr float GRID_OFFSET_Y = 60.0f;

SDL_FRect panelRect(int windowWidth) {
  return SDL_FRect{windowWidth - PANEL_WIDTH - 16.0f, 48.0f, PANEL_WIDTH, PANEL_HEIGHT};
}

SDL_FRect nodeRect(const SkillNodeDef& node, const SDL_FRect& panel) {
  const float x = panel.x + GRID_OFFSET_X + (node.col * (NODE_SIZE + NODE_PADDING));
  const float y = panel.y + GRID_OFFSET_Y + (node.row * (NODE_SIZE + NODE_PADDING));
  return SDL_FRect{x, y, NODE_SIZE, NODE_SIZE};
}

bool pointInRect(int x, int y, const SDL_FRect& rect) {
  return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

bool prerequisitesMet(const SkillTreeComponent& tree, const SkillNodeDef& node) {
  for (int prereq : node.prerequisites) {
    if (tree.unlockedSkills.count(prereq) == 0) {
      return false;
    }
  }
  return true;
}
} // namespace

void SkillTree::handleInput(const bool* keyboardState, int mouseX, int mouseY, bool mousePressed,
                            SkillTreeComponent& tree, const SkillTreeDefinition& definition,
                            int windowWidth) {
  lastMouseX = mouseX;
  lastMouseY = mouseY;

  const bool togglePressed = keyboardState[SDL_SCANCODE_K];
  if (togglePressed && !wasTogglePressed) {
    isOpenFlag = !isOpenFlag;
  }
  wasTogglePressed = togglePressed;

  const bool click = mousePressed && !wasMousePressed;
  wasMousePressed = mousePressed;
  if (!isOpenFlag || !click) {
    return;
  }

  SDL_FRect panel = panelRect(windowWidth);
  if (!pointInRect(mouseX, mouseY, panel)) {
    return;
  }

  for (const SkillNodeDef& node : definition.nodes()) {
    SDL_FRect rect = nodeRect(node, panel);
    if (!pointInRect(mouseX, mouseY, rect)) {
      continue;
    }
    const bool unlocked = tree.unlockedSkills.count(node.id) > 0;
    const bool available = prerequisitesMet(tree, node) && tree.unspentPoints > 0;
    if (!unlocked && available) {
      tree.unspentPoints = std::max(0, tree.unspentPoints - 1);
      tree.unlockedSkills.insert(node.id);
    }
    break;
  }
}

void SkillTree::render(SDL_Renderer* renderer, TTF_Font* font, const SkillTreeComponent& tree,
                       const SkillTreeDefinition& definition, const SkillDatabase& database,
                       int windowWidth, int windowHeight) {
  if (!isOpenFlag) {
    return;
  }

  SDL_FRect panel = panelRect(windowWidth);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 15, 15, 15, 210);
  SDL_RenderFillRect(renderer, &panel);
  SDL_SetRenderDrawColor(renderer, 180, 180, 180, 220);
  SDL_RenderRect(renderer, &panel);

  SDL_Color textColor = {235, 235, 235, 255};
  SDL_Surface* titleSurface = TTF_RenderText_Solid(font, "Skill Tree", 10, textColor);
  SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
  SDL_FRect titleRect = {panel.x + PANEL_PADDING, panel.y + 8.0f,
                         static_cast<float>(titleSurface->w),
                         static_cast<float>(titleSurface->h)};
  SDL_RenderTexture(renderer, titleTexture, nullptr, &titleRect);
  SDL_DestroySurface(titleSurface);
  SDL_DestroyTexture(titleTexture);

  std::string pointsText = "SP: " + std::to_string(tree.unspentPoints);
  SDL_Surface* pointsSurface =
      TTF_RenderText_Solid(font, pointsText.c_str(), pointsText.length(), textColor);
  SDL_Texture* pointsTexture = SDL_CreateTextureFromSurface(renderer, pointsSurface);
  SDL_FRect pointsRect = {panel.x + PANEL_PADDING, panel.y + 28.0f,
                          static_cast<float>(pointsSurface->w),
                          static_cast<float>(pointsSurface->h)};
  SDL_RenderTexture(renderer, pointsTexture, nullptr, &pointsRect);
  SDL_DestroySurface(pointsSurface);
  SDL_DestroyTexture(pointsTexture);

  for (const SkillNodeDef& node : definition.nodes()) {
    SDL_FRect nodeBox = nodeRect(node, panel);
    const float centerX = nodeBox.x + (nodeBox.w / 2.0f);
    const float centerY = nodeBox.y + (nodeBox.h / 2.0f);
    for (int prereq : node.prerequisites) {
      const SkillNodeDef* prereqNode = definition.getNode(prereq);
      if (!prereqNode) {
        continue;
      }
      SDL_FRect prereqBox = nodeRect(*prereqNode, panel);
      const float prereqX = prereqBox.x + (prereqBox.w / 2.0f);
      const float prereqY = prereqBox.y + (prereqBox.h / 2.0f);
      const bool prereqUnlocked = tree.unlockedSkills.count(prereq) > 0;
      SDL_SetRenderDrawColor(renderer, prereqUnlocked ? 120 : 60, prereqUnlocked ? 200 : 60,
                             prereqUnlocked ? 120 : 60, 220);
      SDL_RenderLine(renderer, prereqX, prereqY, centerX, centerY);
    }
  }

  const SkillNodeDef* hovered = nullptr;
  for (const SkillNodeDef& node : definition.nodes()) {
    SDL_FRect rect = nodeRect(node, panel);
    if (pointInRect(lastMouseX, lastMouseY, rect)) {
      hovered = &node;
    }

    const bool unlocked = tree.unlockedSkills.count(node.id) > 0;
    const bool available = prerequisitesMet(tree, node) && tree.unspentPoints > 0;
    if (unlocked) {
      SDL_SetRenderDrawColor(renderer, 70, 160, 90, 240);
    } else if (available) {
      SDL_SetRenderDrawColor(renderer, 200, 160, 60, 240);
    } else {
      SDL_SetRenderDrawColor(renderer, 80, 80, 80, 220);
    }
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 210, 210, 210, 220);
    SDL_RenderRect(renderer, &rect);
  }

  if (hovered) {
    const SkillDef* def = database.getSkill(hovered->id);
    const std::string name = def ? def->name : "Unknown";
    std::string status;
    if (tree.unlockedSkills.count(hovered->id) > 0) {
      status = "Unlocked";
    } else if (prerequisitesMet(tree, *hovered)) {
      status = tree.unspentPoints > 0 ? "Click to learn" : "Needs skill point";
    } else {
      status = "Locked";
    }
    std::string tooltipText = name + " - " + status;
    SDL_Surface* tooltipSurface =
        TTF_RenderText_Solid(font, tooltipText.c_str(), tooltipText.length(), textColor);
    SDL_Texture* tooltipTexture = SDL_CreateTextureFromSurface(renderer, tooltipSurface);
    SDL_FRect tooltipRect = {static_cast<float>(lastMouseX + 12), static_cast<float>(lastMouseY + 12),
                             static_cast<float>(tooltipSurface->w),
                             static_cast<float>(tooltipSurface->h)};
    SDL_FRect tooltipBg = {tooltipRect.x - 6.0f, tooltipRect.y - 4.0f, tooltipRect.w + 12.0f,
                           tooltipRect.h + 8.0f};
    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 220);
    SDL_RenderFillRect(renderer, &tooltipBg);
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 220);
    SDL_RenderRect(renderer, &tooltipBg);
    SDL_RenderTexture(renderer, tooltipTexture, nullptr, &tooltipRect);
    SDL_DestroySurface(tooltipSurface);
    SDL_DestroyTexture(tooltipTexture);
  }
}
