#pragma once

#include "component.h"

class HealthComponent : public Component {
public:
  HealthComponent(int current = 0, int max = 0) : current(current), max(max) {}

public:
  int current;
  int max;
};
