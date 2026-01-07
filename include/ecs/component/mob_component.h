#pragma once

#include "component.h"

class MobComponent : public Component {
public:
  MobComponent(int level, int homeX, int homeY, int regionX, int regionY, int regionWidth,
               int regionHeight, float aggroRange, float leashRange, float speed)
      : level(level), homeX(homeX), homeY(homeY), regionX(regionX), regionY(regionY),
        regionWidth(regionWidth), regionHeight(regionHeight), aggroRange(aggroRange),
        leashRange(leashRange), speed(speed) {}

  int level;
  int homeX;
  int homeY;
  int regionX;
  int regionY;
  int regionWidth;
  int regionHeight;
  float aggroRange;
  float leashRange;
  float speed;
};
