#include "events/event_bus.h"

void EventBus::emitDamageEvent(const DamageEvent& event) {
  damageEvents.push_back(event);
}

void EventBus::emitFloatingTextEvent(const FloatingTextEvent& event) {
  floatingTextEvents.push_back(event);
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
