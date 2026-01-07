#pragma once

#include <vector>

#include "events/events.h"

class EventBus {
public:
  void emitDamageEvent(const DamageEvent& event);
  void emitFloatingTextEvent(const FloatingTextEvent& event);
  void emitRegionEvent(const RegionEvent& event);

  std::vector<DamageEvent> consumeDamageEvents();
  std::vector<FloatingTextEvent> consumeFloatingTextEvents();
  std::vector<RegionEvent> consumeRegionEvents();

private:
  std::vector<DamageEvent> damageEvents;
  std::vector<FloatingTextEvent> floatingTextEvents;
  std::vector<RegionEvent> regionEvents;
};
