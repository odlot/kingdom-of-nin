#include "camera.h"
#include <algorithm>

void Camera::update(Position position) {
  this->position.x = position.x - (this->windowWidth / 2.0f);
  this->position.y = position.y - (this->windowHeight / 2.0f);

  const float maxX = std::max(0.0f, worldWidth - this->windowWidth);
  const float maxY = std::max(0.0f, worldHeight - this->windowHeight);

  this->position.x = std::clamp(this->position.x, 0.0f, maxX);
  this->position.y = std::clamp(this->position.y, 0.0f, maxY);
}
