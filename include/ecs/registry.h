#pragma once

#include "ecs/component/component.h"
#include "ecs/system/system.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

class Registry {
public:
  Registry();
  ~Registry() = default;

  int createEntity();

  template <typename T>
  void registerComponentForEntity(std::unique_ptr<Component> component, int entityId) {
    int componentId = getComponentId<T>();
    if (components.count(componentId) == 0) {
      components[componentId] = std::vector<std::unique_ptr<Component>>();
    }
    components[componentId].push_back(std::move(component));
    indexes[entityId][componentId] = components[componentId].size() - 1;
    if (signatures.count(entityId) == 0) {
      signatures[entityId] = std::bitset<MAX_COMPONENTS>();
    }
    signatures[entityId].set(componentId, true);
    for (auto it = systems.begin(); it != systems.end(); ++it) {
      System* system = it->get();
      if ((signatures[entityId] & system->getSignature()) == system->getSignature()) {
        system->registerEntity(entityId);
      }
    }
  }

  template <typename T> T& getComponent(int entityId) {
    int componentId = getComponentId<T>();
    int index = indexes[entityId][componentId];
    return static_cast<T&>(*this->components[componentId][index]);
  }

  std::vector<std::unique_ptr<System>>::const_iterator systemsBegin() const;
  std::vector<std::unique_ptr<System>>::const_iterator systemsEnd() const;

private:
  template <typename T> void registerComponent();

  template <typename T> int getComponentId();

private:
  int componentId;
  std::unordered_map<std::string, int> componentIds;

  std::unordered_map<int, std::vector<std::unique_ptr<Component>>> components;

  std::vector<std::unique_ptr<System>> systems;

  int entityId;
  // Maps entity ID to its component signature
  std::unordered_map<int, std::bitset<MAX_COMPONENTS>> signatures;

  // Maps entity ID and component ID to index in components vector
  std::unordered_map<int, std::unordered_map<int, int>> indexes;
};

template <typename T> void Registry::registerComponent() {
  const std::type_info& typeInfo = typeid(T);
  int id = componentId++;
  componentIds[typeInfo.name()] = id;
  auto logger = spdlog::get("console");
  if (logger) {
    logger->info("Registered component: {} with id {}", typeInfo.name(), id);
  }
}

template <typename T> int Registry::getComponentId() {
  const std::type_info& typeInfo = typeid(T);
  return componentIds[typeInfo.name()];
}
