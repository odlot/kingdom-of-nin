#include "ui/minimap.h"
#include <algorithm>

Minimap::Minimap(int width, int height, int margin)
    : width(width), height(height), margin(margin) {}

void Minimap::render(SDL_Renderer* renderer, const Map& map, const Position& playerPosition,
                     int windowWidth, int windowHeight, const std::vector<MinimapMarker>& markers) {
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  const float mapWidth = static_cast<float>(map.getWidth());
  const float mapHeight = static_cast<float>(map.getHeight());
  const float scaleX = width / mapWidth;
  const float scaleY = height / mapHeight;

  const float originX = static_cast<float>(windowWidth - width - margin);
  const float originY = static_cast<float>(margin);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
  SDL_FRect bgRect = {originX - 4.0f, originY - 4.0f, static_cast<float>(width + 8),
                      static_cast<float>(height + 8)};
  SDL_RenderFillRect(renderer, &bgRect);

  for (int y = 0; y < map.getHeight(); ++y) {
    for (int x = 0; x < map.getWidth(); ++x) {
      SDL_Color color = {40, 120, 40, 255};
      switch (map.getTile(x, y)) {
      case Tile::Grass:
        color = {40, 120, 40, 255};
        break;
      case Tile::Water:
        color = {30, 60, 160, 255};
        break;
      case Tile::Mountain:
        color = {120, 120, 120, 255};
        break;
      case Tile::Town:
        color = {180, 150, 60, 255};
        break;
      case Tile::DungeonEntrance:
        color = {180, 80, 40, 255};
        break;
      }
      SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 220);
      SDL_FRect pixel = {originX + (x * scaleX), originY + (y * scaleY), std::max(1.0f, scaleX),
                         std::max(1.0f, scaleY)};
      SDL_RenderFillRect(renderer, &pixel);
    }
  }

  const float playerTileX = playerPosition.x / TILE_SIZE;
  const float playerTileY = playerPosition.y / TILE_SIZE;
  SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
  SDL_FRect playerDot = {originX + (playerTileX * scaleX) - 2.0f,
                         originY + (playerTileY * scaleY) - 2.0f, 4.0f, 4.0f};
  SDL_RenderFillRect(renderer, &playerDot);

  for (const MinimapMarker& marker : markers) {
    SDL_SetRenderDrawColor(renderer, marker.color.r, marker.color.g, marker.color.b,
                           marker.color.a);
    SDL_FRect dot = {originX + (marker.tileX * scaleX) - 2.0f,
                     originY + (marker.tileY * scaleY) - 2.0f, 4.0f, 4.0f};
    SDL_RenderFillRect(renderer, &dot);
  }

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
  SDL_FRect border = {originX - 4.0f, originY - 4.0f, static_cast<float>(width + 8),
                      static_cast<float>(height + 8)};
  SDL_RenderRect(renderer, &border);
}
