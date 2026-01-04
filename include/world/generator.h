#pragma once

#include <memory>

#include "world/map.h"

class Generator {
public:
  Generator() = default;
  std::unique_ptr<Map> generate();
};