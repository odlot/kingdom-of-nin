#include "ui/render_utils.h"

#include <cmath>

void drawCircle(SDL_Renderer* renderer, const Position& center, float radius,
                const Position& cameraPosition, SDL_Color color) {
  constexpr int kSegments = 32;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  float prevX = center.x + radius;
  float prevY = center.y;
  constexpr float kPi = 3.14159265f;
  for (int i = 1; i <= kSegments; ++i) {
    const float angle = static_cast<float>(i) * (2.0f * kPi) / static_cast<float>(kSegments);
    const float nextX = center.x + radius * std::cos(angle);
    const float nextY = center.y + radius * std::sin(angle);
    SDL_RenderLine(renderer, prevX - cameraPosition.x, prevY - cameraPosition.y,
                   nextX - cameraPosition.x, nextY - cameraPosition.y);
    prevX = nextX;
    prevY = nextY;
  }
}

void drawFacingArc(SDL_Renderer* renderer, const Position& center, float radius, float facingX,
                   float facingY, float halfAngle, const Position& cameraPosition,
                   SDL_Color color) {
  float fx = facingX;
  float fy = facingY;
  const float facingLength = std::sqrt((fx * fx) + (fy * fy));
  if (facingLength <= 0.001f) {
    fx = 0.0f;
    fy = 1.0f;
  } else {
    fx /= facingLength;
    fy /= facingLength;
  }
  const float centerAngle = std::atan2(fy, fx);
  constexpr int kSegments = 16;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  float prevX = center.x + radius * std::cos(centerAngle - halfAngle);
  float prevY = center.y + radius * std::sin(centerAngle - halfAngle);
  for (int i = 1; i <= kSegments; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kSegments);
    const float angle = centerAngle - halfAngle + (t * halfAngle * 2.0f);
    const float nextX = center.x + radius * std::cos(angle);
    const float nextY = center.y + radius * std::sin(angle);
    SDL_RenderLine(renderer, prevX - cameraPosition.x, prevY - cameraPosition.y,
                   nextX - cameraPosition.x, nextY - cameraPosition.y);
    prevX = nextX;
    prevY = nextY;
  }
  SDL_RenderLine(renderer, center.x - cameraPosition.x, center.y - cameraPosition.y,
                 center.x + radius * std::cos(centerAngle - halfAngle) - cameraPosition.x,
                 center.y + radius * std::sin(centerAngle - halfAngle) - cameraPosition.y);
  SDL_RenderLine(renderer, center.x - cameraPosition.x, center.y - cameraPosition.y,
                 center.x + radius * std::cos(centerAngle + halfAngle) - cameraPosition.x,
                 center.y + radius * std::sin(centerAngle + halfAngle) - cameraPosition.y);
}
