#pragma once

#include "component.h"

class ManaComponent : public Component {
public:
  ManaComponent(int current = 0, int max = 0) : current(current), max(max) {}

public:
  int current;
  int max;
};
