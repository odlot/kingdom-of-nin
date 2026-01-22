#pragma once

#include <vector>

#include "events/events.h"

class EventBus {
public:
  void emitDamageEvent(const DamageEvent& event);
  void emitFloatingTextEvent(const FloatingTextEvent& event);
  void emitRegionEvent(const RegionEvent& event);
  void emitMobKilledEvent(const MobKilledEvent& event);
  void emitItemPickupEvent(const ItemPickupEvent& event);

  std::vector<DamageEvent> consumeDamageEvents();
  std::vector<FloatingTextEvent> consumeFloatingTextEvents();
  std::vector<RegionEvent> consumeRegionEvents();
  std::vector<MobKilledEvent> consumeMobKilledEvents();
  std::vector<ItemPickupEvent> consumeItemPickupEvents();

private:
  std::vector<DamageEvent> damageEvents;
  std::vector<FloatingTextEvent> floatingTextEvents;
  std::vector<RegionEvent> regionEvents;
  std::vector<MobKilledEvent> mobKilledEvents;
  std::vector<ItemPickupEvent> itemPickupEvents;
};
