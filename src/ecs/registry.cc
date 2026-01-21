#include "ecs/registry.h"
#include "SDL3/SDL.h"
#include "ecs/component/buff_component.h"
#include "ecs/component/collision_component.h"
#include "ecs/component/equipment_component.h"
#include "ecs/component/graphic_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/inventory_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/loot_component.h"
#include "ecs/component/mana_component.h"
#include "ecs/component/mob_component.h"
#include "ecs/component/movement_component.h"
#include "ecs/component/projectile_component.h"
#include "ecs/component/pushback_component.h"
#include "ecs/component/skill_bar_component.h"
#include "ecs/component/skill_tree_component.h"
#include "ecs/component/stats_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/system/graphic_system.h"
#include "ecs/system/movement_system.h"
#include "ecs/system/pushback_system.h"
#include <spdlog/spdlog.h>

Registry::Registry()
    : componentId(0), componentIds(std::unordered_map<std::string, int>()),
      systems(std::vector<std::unique_ptr<System>>()),
      components(std::unordered_map<int, std::vector<std::unique_ptr<Component>>>()), entityId(0),
      signatures(std::unordered_map<int, std::bitset<MAX_COMPONENTS>>()),
      indexes(std::unordered_map<int, std::unordered_map<int, int>>()) {
  registerComponent<TransformComponent>();
  registerComponent<GraphicComponent>();
  registerComponent<MovementComponent>();
  registerComponent<CollisionComponent>();
  registerComponent<HealthComponent>();
  registerComponent<ManaComponent>();
  registerComponent<LevelComponent>();
  registerComponent<InventoryComponent>();
  registerComponent<EquipmentComponent>();
  registerComponent<StatsComponent>();
  registerComponent<LootComponent>();
  registerComponent<MobComponent>();
  registerComponent<ProjectileComponent>();
  registerComponent<PushbackComponent>();
  registerComponent<BuffComponent>();
  registerComponent<SkillBarComponent>();
  registerComponent<SkillTreeComponent>();

  {
    auto signature = std::bitset<MAX_COMPONENTS>();
    signature.set(getComponentId<TransformComponent>(), true);
    signature.set(getComponentId<GraphicComponent>(), true);
    this->systems.push_back(std::make_unique<GraphicSystem>(*this, signature));
  }

  {
    auto signature = std::bitset<MAX_COMPONENTS>();
    signature.set(getComponentId<TransformComponent>(), true);
    signature.set(getComponentId<MovementComponent>(), true);
    signature.set(getComponentId<CollisionComponent>(), true);
    this->systems.push_back(std::make_unique<MovementSystem>(*this, signature));
  }

  {
    auto signature = std::bitset<MAX_COMPONENTS>();
    signature.set(getComponentId<TransformComponent>(), true);
    signature.set(getComponentId<CollisionComponent>(), true);
    signature.set(getComponentId<PushbackComponent>(), true);
    this->systems.push_back(std::make_unique<PushbackSystem>(*this, signature));
  }
}

int Registry::createEntity() {
  return this->entityId++;
}

std::vector<std::unique_ptr<System>>::const_iterator Registry::systemsBegin() const {
  return systems.begin();
}

std::vector<std::unique_ptr<System>>::const_iterator Registry::systemsEnd() const {
  return systems.end();
}
