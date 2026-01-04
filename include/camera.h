#pragma once

#include "ecs/position.h"

class Camera {
public:
  Camera(Position position, int windowWidth, int windowHeight, float worldWidth, float worldHeight)
      : position(position), windowWidth(windowWidth), windowHeight(windowHeight),
        worldWidth(worldWidth), worldHeight(worldHeight) {}

  void update(Position position);
  const Position& getPosition() const { return position; }

private:
  Position position;
  int windowWidth;
  int windowHeight;
  float worldWidth;
  float worldHeight;
};
