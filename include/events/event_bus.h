#pragma once

#include <vector>

#include "events/events.h"

class EventBus {
public:
  void emitDamageEvent(const DamageEvent& event);
  void emitFloatingTextEvent(const FloatingTextEvent& event);

  std::vector<DamageEvent> consumeDamageEvents();
  std::vector<FloatingTextEvent> consumeFloatingTextEvents();

private:
  std::vector<DamageEvent> damageEvents;
  std::vector<FloatingTextEvent> floatingTextEvents;
};
