#pragma once

#include "ecs/component/component.h"
#include <string>

class NpcComponent : public Component {
public:
  NpcComponent(std::string name, std::string dialogLine)
      : name(std::move(name)), dialogLine(std::move(dialogLine)) {}

  std::string name;
  std::string dialogLine;
};
