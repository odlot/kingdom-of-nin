#pragma once

#include "ecs/component/component.h"
#include <vector>

class ShopComponent : public Component {
public:
  explicit ShopComponent(std::vector<int> stock) : stock(std::move(stock)) {}

  std::vector<int> stock;
};
