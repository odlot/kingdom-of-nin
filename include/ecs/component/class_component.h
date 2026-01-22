#pragma once

#include "ecs/component/component.h"
#include "items/item.h"

class ClassComponent : public Component {
public:
  explicit ClassComponent(CharacterClass characterClass = CharacterClass::Any)
      : characterClass(characterClass) {}

  CharacterClass characterClass = CharacterClass::Any;
};
