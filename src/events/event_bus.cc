#include "events/event_bus.h"

void EventBus::emitDamageEvent(const DamageEvent& event) {
  damageEvents.push_back(event);
}

void EventBus::emitFloatingTextEvent(const FloatingTextEvent& event) {
  floatingTextEvents.push_back(event);
}

void EventBus::emitRegionEvent(const RegionEvent& event) {
  regionEvents.push_back(event);
}

void EventBus::emitMobKilledEvent(const MobKilledEvent& event) {
  mobKilledEvents.push_back(event);
}

void EventBus::emitItemPickupEvent(const ItemPickupEvent& event) {
  itemPickupEvents.push_back(event);
}

std::vector<DamageEvent> EventBus::consumeDamageEvents() {
  std::vector<DamageEvent> events = std::move(damageEvents);
  damageEvents.clear();
  return events;
}

std::vector<FloatingTextEvent> EventBus::consumeFloatingTextEvents() {
  std::vector<FloatingTextEvent> events = std::move(floatingTextEvents);
  floatingTextEvents.clear();
  return events;
}

std::vector<RegionEvent> EventBus::consumeRegionEvents() {
  std::vector<RegionEvent> events = std::move(regionEvents);
  regionEvents.clear();
  return events;
}

std::vector<MobKilledEvent> EventBus::consumeMobKilledEvents() {
  std::vector<MobKilledEvent> events = std::move(mobKilledEvents);
  mobKilledEvents.clear();
  return events;
}

std::vector<ItemPickupEvent> EventBus::consumeItemPickupEvents() {
  std::vector<ItemPickupEvent> events = std::move(itemPickupEvents);
  itemPickupEvents.clear();
  return events;
}
