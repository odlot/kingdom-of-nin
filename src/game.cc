#include "game.h"
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
#include "ecs/component/stats_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/system/graphic_system.h"
#include "ecs/system/movement_system.h"
#include "events/events.h"
#include "ui/floating_text_system.h"
#include "ui/inventory.h"
#include "ui/minimap.h"
#include "world/generator.h"
#include "world/region.h"
#include "world/tile.h"
#include <SDL3/SDL_keyboard.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <optional>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>

constexpr std::string WINDOW_TITLE = "Kingdom of Nin";
constexpr int MINIMAP_WIDTH = 160;
constexpr int MINIMAP_HEIGHT = 120;
constexpr int MINIMAP_MARGIN = 12;
constexpr float ATTACK_RANGE = 56.0f;
constexpr float ATTACK_COOLDOWN_SECONDS = 0.3f;
constexpr float LOOT_PICKUP_RANGE = 40.0f;

namespace {
std::optional<Coordinate> firstWalkableInRegion(const Map& map, const Region& region) {
  for (int y = region.y; y < region.y + region.height; ++y) {
    for (int x = region.x; x < region.x + region.width; ++x) {
      if (map.isWalkable(x, y)) {
        return Coordinate(x, y);
      }
    }
  }
  return std::nullopt;
}

std::string regionName(RegionType type) {
  switch (type) {
  case RegionType::StartingZone:
    return "Starting Zone";
  case RegionType::SpawnRegion:
    return "Spawn Region";
  case RegionType::GoblinCamp:
    return "Goblin Camp";
  case RegionType::DungeonEntrance:
    return "Dungeon Entrance";
  }
  return "Region";
}

const char* mobTypeName(MobType type) {
  switch (type) {
  case MobType::Goblin:
    return "Goblin";
  case MobType::GoblinArcher:
    return "Goblin Archer";
  case MobType::GoblinBrute:
    return "Goblin Brute";
  }
  return "Mob";
}

int regionIndexAt(const Map& map, int tileX, int tileY) {
  const std::vector<Region>& regions = map.getRegions();
  for (std::size_t i = 0; i < regions.size(); ++i) {
    if (regions[i].contains(tileX, tileY)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void drawCircle(SDL_Renderer* renderer, const Position& center, float radius,
                const Position& cameraPosition, SDL_Color color) {
  constexpr int kSegments = 32;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  float prevX = center.x + radius;
  float prevY = center.y;
  constexpr float kPi = 3.14159265f;
  for (int i = 1; i <= kSegments; ++i) {
    const float angle = static_cast<float>(i) * (2.0f * kPi) / static_cast<float>(kSegments);
    const float nextX = center.x + radius * std::cos(angle);
    const float nextY = center.y + radius * std::sin(angle);
    SDL_RenderLine(renderer, prevX - cameraPosition.x, prevY - cameraPosition.y,
                   nextX - cameraPosition.x, nextY - cameraPosition.y);
    prevX = nextX;
    prevY = nextY;
  }
}

} // namespace

int computeAttackPower(const StatsComponent& stats, const EquipmentComponent& equipment,
                       const ItemDatabase& database) {
  int total = stats.baseAttackPower + stats.strength;
  for (const auto& entry : equipment.equipped) {
    const ItemDef* def = database.getItem(entry.second.itemId);
    if (!def) {
      continue;
    }
    total += def->stats.attackPower;
  }
  return total;
}

float squaredDistance(const Position& a, const Position& b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return (dx * dx) + (dy * dy);
}

void applyLevelUps(LevelComponent& level, StatsComponent& stats) {
  while (level.experience >= level.nextLevelExperience) {
    level.experience -= level.nextLevelExperience;
    level.level += 1;
    level.nextLevelExperience += 100;
    stats.unspentPoints += 5;
  }
}

bool equipItemByIndex(InventoryComponent& inventory, EquipmentComponent& equipment,
                      const ItemDatabase& database, std::size_t index) {
  std::optional<ItemInstance> item = inventory.takeItemAt(index);
  if (!item.has_value()) {
    return false;
  }
  const ItemDef* def = database.getItem(item->itemId);
  if (!def) {
    inventory.addItem(*item);
    return false;
  }

  auto equippedIt = equipment.equipped.find(def->slot);
  if (equippedIt != equipment.equipped.end()) {
    if (!inventory.addItem(equippedIt->second)) {
      inventory.addItem(*item);
      return false;
    }
    equipment.equipped.erase(equippedIt);
  }

  equipment.equipped[def->slot] = *item;
  return true;
}

Position centerForEntity(const TransformComponent& transform, const CollisionComponent& collision) {
  return Position(transform.position.x + (collision.width / 2.0f),
                  transform.position.y + (collision.height / 2.0f));
}

bool createLootEntity(Registry& registry, const Position& position, int itemId,
                      std::vector<int>& lootEntityIds) {
  int entityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), entityId);
  registry.registerComponentForEntity<GraphicComponent>(
      std::make_unique<GraphicComponent>(position, SDL_Color({230, 210, 80, 255})), entityId);
  registry.registerComponentForEntity<CollisionComponent>(
      std::make_unique<CollisionComponent>(32.0f, 32.0f, false), entityId);
  registry.registerComponentForEntity<LootComponent>(std::make_unique<LootComponent>(itemId),
                                                     entityId);
  lootEntityIds.push_back(entityId);
  return true;
}

bool isBlockedByMap(const Map& map, const CollisionComponent& collision, float nextX, float nextY) {
  const float left = nextX;
  const float right = nextX + collision.width - 1.0f;
  const float top = nextY;
  const float bottom = nextY + collision.height - 1.0f;

  const int tileLeft = static_cast<int>(left / TILE_SIZE);
  const int tileRight = static_cast<int>(right / TILE_SIZE);
  const int tileTop = static_cast<int>(top / TILE_SIZE);
  const int tileBottom = static_cast<int>(bottom / TILE_SIZE);

  return !map.isWalkable(tileLeft, tileTop) || !map.isWalkable(tileRight, tileTop) ||
         !map.isWalkable(tileLeft, tileBottom) || !map.isWalkable(tileRight, tileBottom);
}

void moveEntityToward(const Map& map, TransformComponent& transform,
                      const CollisionComponent& collision, float speed, const Position& target,
                      float dt) {
  float dx = target.x - transform.position.x;
  float dy = target.y - transform.position.y;
  const float length = std::sqrt((dx * dx) + (dy * dy));
  if (length <= 0.001f) {
    return;
  }
  dx /= length;
  dy /= length;
  const float newX = transform.position.x + (dx * speed * dt);
  const float newY = transform.position.y + (dy * speed * dt);

  if (!isBlockedByMap(map, collision, newX, transform.position.y)) {
    transform.position.x = newX;
  }
  if (!isBlockedByMap(map, collision, transform.position.x, newY)) {
    transform.position.y = newY;
  }
}

Game::Game() {
  auto logger = spdlog::get("console");
  if (!logger) {
    logger = spdlog::stdout_color_mt("console");
  }
  logger->info("Initializing SDL");
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    logger->error("SDL could not initialize! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error("SDL Initialization failed");
  }
  logger->info("Creating window and renderer");
  this->window = SDL_CreateWindow(WINDOW_TITLE.c_str(), WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (!this->window) {
    logger->error("Window could not be created! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error("SDL Window creation failed");
  }
  SDL_ShowWindow(this->window);
  this->renderer = SDL_CreateRenderer(this->window, nullptr);
  if (!this->renderer) {
    logger->error("Renderer could not be created! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error("SDL Renderer creation failed");
  }
  this->registry = std::make_unique<Registry>();
  this->itemDatabase = std::make_unique<ItemDatabase>();
  this->eventBus = std::make_unique<EventBus>();
  this->floatingTextSystem = std::make_unique<FloatingTextSystem>(*this->eventBus);
  this->respawnSystem = std::make_unique<RespawnSystem>();
  this->inventoryUi = std::make_unique<Inventory>();
  this->characterStats = std::make_unique<CharacterStats>();
  if (!TTF_Init()) {
    logger->error("SDL TTF could not initialize! SDL_Error: {}", SDL_GetError());
    throw std::runtime_error("SDL TTF Initialization failed");
  }
  const char* base = SDL_GetBasePath();
  logger->info("SDL Base Path: {}", base);
  std::filesystem::path path = base ? base : ".";
  std::filesystem::path assets = path / "assets";
  std::string fontPath = assets.string() + "/fonts/arial.ttf";
  this->font = TTF_OpenFont(fontPath.c_str(), 14);

  Generator generator;
  this->map = generator.generate();
  this->map->print();

  Coordinate start = this->map->getStartingPosition();
  Position playerPosition(start.x * TILE_SIZE, start.y * TILE_SIZE);
  this->playerEntityId = this->registry->createEntity();
  this->registry->registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(playerPosition), this->playerEntityId);
  this->registry->registerComponentForEntity<MovementComponent>(
      std::make_unique<MovementComponent>(100.0f), this->playerEntityId);
  this->registry->registerComponentForEntity<GraphicComponent>(
      std::make_unique<GraphicComponent>(playerPosition, SDL_Color({240, 240, 240, 255})),
      this->playerEntityId);
  this->registry->registerComponentForEntity<CollisionComponent>(
      std::make_unique<CollisionComponent>(32.0f, 32.0f, true), this->playerEntityId);
  this->registry->registerComponentForEntity<HealthComponent>(
      std::make_unique<HealthComponent>(100, 100), this->playerEntityId);
  this->registry->registerComponentForEntity<ManaComponent>(std::make_unique<ManaComponent>(50, 50),
                                                            this->playerEntityId);
  this->registry->registerComponentForEntity<LevelComponent>(
      std::make_unique<LevelComponent>(1, 0, 100), this->playerEntityId);
  this->registry->registerComponentForEntity<InventoryComponent>(
      std::make_unique<InventoryComponent>(), this->playerEntityId);
  this->registry->registerComponentForEntity<EquipmentComponent>(
      std::make_unique<EquipmentComponent>(), this->playerEntityId);
  this->registry->registerComponentForEntity<StatsComponent>(std::make_unique<StatsComponent>(),
                                                             this->playerEntityId);

  {
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    ItemInstance swordInstance{1};
    if (inventory.addItem(swordInstance)) {
      equipItemByIndex(inventory, equipment, *this->itemDatabase, 0);
    }
  }

  { // Spawn loot items near the starting zone
    const std::array<int, 4> lootItems = {1, 2, 3, 4};
    const std::array<std::pair<int, int>, 6> offsets = {
        std::make_pair(-2, 0), std::make_pair(2, 0),   std::make_pair(0, -2),
        std::make_pair(0, 2),  std::make_pair(-3, -1), std::make_pair(3, 1)};
    int lootIndex = 0;
    for (const auto& offset : offsets) {
      int tileX = start.x + offset.first;
      int tileY = start.y + offset.second;
      if (!this->map->isWalkable(tileX, tileY)) {
        continue;
      }
      Position lootPosition(tileX * TILE_SIZE, tileY * TILE_SIZE);
      createLootEntity(*this->registry, lootPosition, lootItems[lootIndex], this->lootEntityIds);
      lootIndex = (lootIndex + 1) % static_cast<int>(lootItems.size());
    }
  }

  { // Spawn goblins inside spawn regions
    this->respawnSystem->initialize(*this->map, *this->registry, this->mobEntityIds);
  }

  const float worldWidth = static_cast<float>(this->map->getWidth() * TILE_SIZE);
  const float worldHeight = static_cast<float>(this->map->getHeight() * TILE_SIZE);
  this->camera = std::make_unique<Camera>(playerPosition, WINDOW_WIDTH, WINDOW_HEIGHT, worldWidth,
                                          worldHeight);
  this->camera->update(playerPosition);
  this->minimap = std::make_unique<Minimap>(MINIMAP_WIDTH, MINIMAP_HEIGHT, MINIMAP_MARGIN);
}

Game::~Game() {
  if (this->font) {
    TTF_CloseFont(this->font);
  }
  SDL_DestroyRenderer(this->renderer);
  SDL_DestroyWindow(this->window);
  SDL_Quit();
}

bool Game::isRunning() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      running = false;
    }
  }
  return running;
}

void Game::update(float dt) {
  // Update game logic here
  if (this->attackCooldownRemaining > 0.0f) {
    this->attackCooldownRemaining = std::max(0.0f, this->attackCooldownRemaining - dt);
  }
  this->floatingTextSystem->update(dt);
  this->playerHitFlashTimer = std::max(0.0f, this->playerHitFlashTimer - dt);
  this->respawnSystem->update(dt, *this->map, *this->registry, this->mobEntityIds);

  int x = 0;
  int y = 0;
  const bool* keyboardState = SDL_GetKeyboardState(nullptr);
  if (keyboardState[SDL_SCANCODE_LEFT]) {
    x = -1;
  }
  if (keyboardState[SDL_SCANCODE_DOWN]) {
    y = 1;
  }
  if (keyboardState[SDL_SCANCODE_UP]) {
    y = -1;
  }
  if (keyboardState[SDL_SCANCODE_RIGHT]) {
    x = 1;
  }
  auto result = std::make_pair(x, y);

  const bool attackPressed = keyboardState[SDL_SCANCODE_SPACE];
  const bool pickupPressed = keyboardState[SDL_SCANCODE_F];
  const bool debugPressed = keyboardState[SDL_SCANCODE_D];
  float mouseX = 0.0f;
  float mouseY = 0.0f;
  const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
  const bool mousePressed = (mouseState & SDL_BUTTON_LMASK) != 0;
  {
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    this->inventoryUi->handleInput(keyboardState, static_cast<int>(mouseX),
                                   static_cast<int>(mouseY), mousePressed, inventory, equipment,
                                   *this->itemDatabase);
  }
  {
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    this->characterStats->handleInput(static_cast<int>(mouseX), static_cast<int>(mouseY),
                                      mousePressed, stats, this->inventoryUi->isStatsVisible());
  }

  {
    LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    applyLevelUps(level, stats);
  }
  if (pickupPressed && !this->wasPickupPressed) {
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);
    const float pickupRangeSquared = LOOT_PICKUP_RANGE * LOOT_PICKUP_RANGE;

    int closestLootId = -1;
    float closestDist = pickupRangeSquared;
    for (int lootId : this->lootEntityIds) {
      const TransformComponent& lootTransform =
          this->registry->getComponent<TransformComponent>(lootId);
      const Position lootCenter(lootTransform.position.x + (TILE_SIZE / 2.0f),
                                lootTransform.position.y + (TILE_SIZE / 2.0f));
      const float dist = squaredDistance(playerCenter, lootCenter);
      if (dist <= closestDist) {
        closestDist = dist;
        closestLootId = lootId;
      }
    }

    if (closestLootId != -1) {
      LootComponent& loot = this->registry->getComponent<LootComponent>(closestLootId);
      ItemInstance item{loot.itemId};
      if (inventory.addItem(item)) {
        const ItemDef* def = this->itemDatabase->getItem(item.itemId);
        std::string name = def ? def->name : "Item";
        this->eventBus->emitFloatingTextEvent(FloatingTextEvent{"Picked up " + name, playerCenter});
        GraphicComponent& graphic = this->registry->getComponent<GraphicComponent>(closestLootId);
        graphic.color = SDL_Color({0, 0, 0, 0});
        TransformComponent& transform =
            this->registry->getComponent<TransformComponent>(closestLootId);
        transform.position = Position(-1000.0f, -1000.0f);
        this->lootEntityIds.erase(
            std::remove(this->lootEntityIds.begin(), this->lootEntityIds.end(), closestLootId),
            this->lootEntityIds.end());
      }
    }
  }
  this->wasPickupPressed = pickupPressed;
  if (debugPressed && !this->wasDebugPressed) {
    this->showDebugMobRanges = !this->showDebugMobRanges;
  }
  this->wasDebugPressed = debugPressed;
  if (attackPressed && !this->wasAttackPressed && this->attackCooldownRemaining <= 0.0f) {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const StatsComponent& stats =
        this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const int attackPower = computeAttackPower(stats, equipment, *this->itemDatabase);

    const Position playerCenter(playerTransform.position.x + (playerCollision.width / 2.0f),
                                playerTransform.position.y + (playerCollision.height / 2.0f));
    const float rangeSquared = ATTACK_RANGE * ATTACK_RANGE;

    for (int mobEntityId : this->mobEntityIds) {
      HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
      if (mobHealth.current <= 0) {
        continue;
      }
      if (this->respawnSystem->isSpawning(mobEntityId)) {
        continue;
      }
      const TransformComponent& mobTransform =
          this->registry->getComponent<TransformComponent>(mobEntityId);
      const Position mobCenter(mobTransform.position.x + (TILE_SIZE / 2.0f),
                               mobTransform.position.y + (TILE_SIZE / 2.0f));
      if (squaredDistance(playerCenter, mobCenter) > rangeSquared) {
        continue;
      }
      mobHealth.current = std::max(0, mobHealth.current - attackPower);
      this->eventBus->emitDamageEvent(
          DamageEvent{this->playerEntityId, mobEntityId, attackPower, mobCenter});
      this->eventBus->emitFloatingTextEvent(
          FloatingTextEvent{std::to_string(attackPower), mobCenter});
      if (mobHealth.current == 0) {
        const MobComponent& mob = this->registry->getComponent<MobComponent>(mobEntityId);
        level.experience += mob.experience;
        this->eventBus->emitFloatingTextEvent(
            FloatingTextEvent{"XP +" + std::to_string(mob.experience), mobCenter});
        GraphicComponent& mobGraphic = this->registry->getComponent<GraphicComponent>(mobEntityId);
        mobGraphic.color = SDL_Color({80, 80, 80, 255});
      }
    }

    this->attackCooldownRemaining = ATTACK_COOLDOWN_SECONDS;
  }
  this->wasAttackPressed = attackPressed;

  // spdlog::get("console")->info("Input direction: ({}, {})", result.first,
  //                               result.second);
  //  TODO: Refactor system iteration based on whether update or render is
  //  needed, implement to an interface?
  for (auto it = this->registry->systemsBegin(); it != this->registry->systemsEnd(); ++it) {
    MovementSystem* movementSystem = dynamic_cast<MovementSystem*>((*it).get());
    if (movementSystem) {
      movementSystem->update(result, dt, *this->map);
    }
  }

  {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);
    const int playerTileX = static_cast<int>(playerCenter.x / TILE_SIZE);
    const int playerTileY = static_cast<int>(playerCenter.y / TILE_SIZE);

    for (int mobEntityId : this->mobEntityIds) {
      const HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
      if (mobHealth.current <= 0) {
        continue;
      }
      if (this->respawnSystem->isSpawning(mobEntityId)) {
        continue;
      }
      TransformComponent& mobTransform =
          this->registry->getComponent<TransformComponent>(mobEntityId);
      const CollisionComponent& mobCollision =
          this->registry->getComponent<CollisionComponent>(mobEntityId);
      MobComponent& mob = this->registry->getComponent<MobComponent>(mobEntityId);
      const Position mobCenter = centerForEntity(mobTransform, mobCollision);
      const bool playerInRegion =
          playerTileX >= mob.regionX && playerTileX < mob.regionX + mob.regionWidth &&
          playerTileY >= mob.regionY && playerTileY < mob.regionY + mob.regionHeight;
      const float distToPlayer = squaredDistance(mobCenter, playerCenter);

      const Position homePosition(mob.homeX * TILE_SIZE, mob.homeY * TILE_SIZE);
      const Position homeCenter(homePosition.x + (mobCollision.width / 2.0f),
                                homePosition.y + (mobCollision.height / 2.0f));
      const float distToHome = squaredDistance(mobCenter, homeCenter);

      std::optional<Position> target;
      if (playerInRegion && distToPlayer <= (mob.aggroRange * mob.aggroRange)) {
        target = playerTransform.position;
      } else if (distToHome > 4.0f) {
        target = homePosition;
      }

      if (target.has_value()) {
        moveEntityToward(*this->map, mobTransform, mobCollision, mob.speed, *target, dt);
      }

      mob.attackTimer = std::max(0.0f, mob.attackTimer - dt);
      if (mob.attackTimer <= 0.0f) {
        const float attackRangeSquared = mob.attackRange * mob.attackRange;
        if (squaredDistance(mobCenter, playerCenter) <= attackRangeSquared) {
          HealthComponent& playerHealth =
              this->registry->getComponent<HealthComponent>(this->playerEntityId);
          if (playerHealth.current > 0) {
            playerHealth.current = std::max(0, playerHealth.current - mob.attackDamage);
            this->eventBus->emitDamageEvent(
                DamageEvent{mobEntityId, this->playerEntityId, mob.attackDamage, playerCenter});
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{"-" + std::to_string(mob.attackDamage), playerCenter});
            this->playerHitFlashTimer = 0.2f;
            mob.attackTimer = mob.attackCooldown;
          }
        }
      }
    }
  }

  const TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  Position playerCenter(playerTransform.position.x + (playerCollision.width / 2.0f),
                        playerTransform.position.y + (playerCollision.height / 2.0f));
  const int playerTileX = static_cast<int>(playerCenter.x / TILE_SIZE);
  const int playerTileY = static_cast<int>(playerCenter.y / TILE_SIZE);
  const int currentRegionIndex = regionIndexAt(*this->map, playerTileX, playerTileY);
  if (currentRegionIndex != this->lastRegionIndex) {
    if (this->lastRegionIndex >= 0) {
      const Region& previousRegion = this->map->getRegions()[this->lastRegionIndex];
      this->eventBus->emitRegionEvent(
          RegionEvent{RegionTransition::Leave, regionName(previousRegion.type), playerCenter});
    }
    if (currentRegionIndex >= 0) {
      const Region& currentRegion = this->map->getRegions()[currentRegionIndex];
      this->eventBus->emitRegionEvent(
          RegionEvent{RegionTransition::Enter, regionName(currentRegion.type), playerCenter});
    }
    this->lastRegionIndex = currentRegionIndex;
  }
  this->camera->update(playerCenter);
}

void Game::render() {
  SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
  SDL_RenderClear(this->renderer);
  const Position& cameraPosition = this->camera->getPosition();
  float mouseX = 0.0f;
  float mouseY = 0.0f;
  SDL_GetMouseState(&mouseX, &mouseY);

  { // Draw map tiles within the camera view
    const int startTileX = std::max(0, static_cast<int>(cameraPosition.x / TILE_SIZE));
    const int startTileY = std::max(0, static_cast<int>(cameraPosition.y / TILE_SIZE));
    const int endTileX = std::min(this->map->getWidth() - 1,
                                  static_cast<int>((cameraPosition.x + WINDOW_WIDTH) / TILE_SIZE));
    const int endTileY = std::min(this->map->getHeight() - 1,
                                  static_cast<int>((cameraPosition.y + WINDOW_HEIGHT) / TILE_SIZE));

    for (int y = startTileY; y <= endTileY; ++y) {
      for (int x = startTileX; x <= endTileX; ++x) {
        SDL_Color color = {0, 0, 0, 255};
        switch (this->map->getTile(x, y)) {
        case Tile::Grass:
          color = {40, 120, 40, 255};
          break;
        case Tile::Water:
          color = {30, 60, 160, 255};
          break;
        case Tile::Mountain:
          color = {120, 120, 120, 255};
          break;
        case Tile::Town:
          color = {180, 150, 60, 255};
          break;
        case Tile::DungeonEntrance:
          color = {180, 80, 40, 255};
          break;
        }
        SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
        SDL_FRect tileRect = {static_cast<float>(x * TILE_SIZE) - cameraPosition.x,
                              static_cast<float>(y * TILE_SIZE) - cameraPosition.y,
                              static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
        SDL_RenderFillRect(this->renderer, &tileRect);
      }
    }
  }
  for (auto it = this->registry->systemsBegin(); it != this->registry->systemsEnd(); ++it) {
    GraphicSystem* graphicSystem = dynamic_cast<GraphicSystem*>((*it).get());
    if (graphicSystem) {
      graphicSystem->render(this->renderer, cameraPosition);
    }
  }

  { // Mob HP bars
    for (int mobEntityId : this->mobEntityIds) {
      const HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
      if (mobHealth.max <= 0) {
        continue;
      }
      if (this->respawnSystem->isSpawning(mobEntityId)) {
        continue;
      }
      const TransformComponent& mobTransform =
          this->registry->getComponent<TransformComponent>(mobEntityId);
      const float healthRatio = std::clamp(
          static_cast<float>(mobHealth.current) / static_cast<float>(mobHealth.max), 0.0f, 1.0f);
      SDL_FRect barBg = {mobTransform.position.x - cameraPosition.x,
                         mobTransform.position.y - cameraPosition.y - 8.0f,
                         static_cast<float>(TILE_SIZE), 4.0f};
      SDL_SetRenderDrawColor(this->renderer, 20, 20, 20, 220);
      SDL_RenderFillRect(this->renderer, &barBg);

      SDL_FRect barFill = barBg;
      barFill.w = barBg.w * healthRatio;
      SDL_SetRenderDrawColor(this->renderer, 200, 40, 40, 255);
      SDL_RenderFillRect(this->renderer, &barFill);
    }
  }

  { // Mob hover labels
    for (int mobEntityId : this->mobEntityIds) {
      const HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
      if (mobHealth.current <= 0) {
        continue;
      }
      if (this->respawnSystem->isSpawning(mobEntityId)) {
        continue;
      }
      const TransformComponent& mobTransform =
          this->registry->getComponent<TransformComponent>(mobEntityId);
      const SDL_FRect mobRect = {mobTransform.position.x - cameraPosition.x,
                                 mobTransform.position.y - cameraPosition.y,
                                 static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
      if (mouseX < mobRect.x || mouseX > mobRect.x + mobRect.w || mouseY < mobRect.y ||
          mouseY > mobRect.y + mobRect.h) {
        continue;
      }
      const MobComponent& mob = this->registry->getComponent<MobComponent>(mobEntityId);
      const char* label = mobTypeName(mob.type);
      SDL_Color textColor = {245, 245, 245, 255};
      SDL_Surface* surface = TTF_RenderText_Solid(this->font, label, std::strlen(label), textColor);
      SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
      SDL_FRect textRect = {mobRect.x, mobRect.y - 18.0f, static_cast<float>(surface->w),
                            static_cast<float>(surface->h)};
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
      SDL_FRect bgRect = {textRect.x - 4.0f, textRect.y - 2.0f, textRect.w + 8.0f,
                          textRect.h + 4.0f};
      SDL_RenderFillRect(this->renderer, &bgRect);
      SDL_RenderTexture(this->renderer, texture, nullptr, &textRect);
      SDL_DestroySurface(surface);
      SDL_DestroyTexture(texture);
      break;
    }
  }

  { // Floating damage text
    this->floatingTextSystem->render(this->renderer, cameraPosition, this->font);
  }

  if (this->showDebugMobRanges) {
    for (int mobEntityId : this->mobEntityIds) {
      const HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
      if (mobHealth.current <= 0) {
        continue;
      }
      const TransformComponent& mobTransform =
          this->registry->getComponent<TransformComponent>(mobEntityId);
      const CollisionComponent& mobCollision =
          this->registry->getComponent<CollisionComponent>(mobEntityId);
      const MobComponent& mob = this->registry->getComponent<MobComponent>(mobEntityId);
      const Position mobCenter = centerForEntity(mobTransform, mobCollision);
      drawCircle(this->renderer, mobCenter, mob.aggroRange, cameraPosition,
                 SDL_Color{240, 60, 60, 180});
      drawCircle(this->renderer, mobCenter, mob.leashRange, cameraPosition,
                 SDL_Color{60, 120, 240, 180});
    }
  }

  { // HUD: Render basic player stats
    const HealthComponent& health =
        this->registry->getComponent<HealthComponent>(this->playerEntityId);
    const ManaComponent& mana = this->registry->getComponent<ManaComponent>(this->playerEntityId);
    const LevelComponent& level =
        this->registry->getComponent<LevelComponent>(this->playerEntityId);
    std::ostringstream hud;
    const StatsComponent& stats =
        this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    int attackPower = computeAttackPower(stats, equipment, *this->itemDatabase);
    hud << "HP " << health.current << "/" << health.max << "  MP " << mana.current << "/"
        << mana.max << "  AP " << attackPower << "  LV " << level.level << " XP "
        << level.experience << "/" << level.nextLevelExperience;
    std::string debugText = hud.str();
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
    SDL_FRect hudBg = {8, 8, 320, 28};
    SDL_RenderFillRect(this->renderer, &hudBg);
    SDL_Surface* surface =
        TTF_RenderText_Solid(this->font, debugText.c_str(), debugText.length(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    SDL_FRect textRect = {12, 12, static_cast<float>(surface->w), static_cast<float>(surface->h)};
    SDL_RenderTexture(this->renderer, texture, nullptr, &textRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
  }

  {
    const InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    this->inventoryUi->render(this->renderer, this->font, inventory, equipment,
                              *this->itemDatabase);
  }

  { // Minimap overlay
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    this->minimap->render(this->renderer, *this->map, playerTransform.position, WINDOW_WIDTH,
                          WINDOW_HEIGHT);
  }

  { // Character stats overlay
    const HealthComponent& health =
        this->registry->getComponent<HealthComponent>(this->playerEntityId);
    const ManaComponent& mana = this->registry->getComponent<ManaComponent>(this->playerEntityId);
    const LevelComponent& level =
        this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const StatsComponent& stats =
        this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const int attackPower = computeAttackPower(stats, equipment, *this->itemDatabase);
    this->characterStats->render(this->renderer, this->font, WINDOW_WIDTH, health, mana, level,
                                 attackPower, stats.strength, stats.dexterity, stats.intellect,
                                 stats.luck, stats.unspentPoints,
                                 this->inventoryUi->isStatsVisible());
  }

  { // Player hit flash
    if (this->playerHitFlashTimer > 0.0f) {
      const TransformComponent& playerTransform =
          this->registry->getComponent<TransformComponent>(this->playerEntityId);
      const CollisionComponent& playerCollision =
          this->registry->getComponent<CollisionComponent>(this->playerEntityId);
      const float alpha = std::clamp(this->playerHitFlashTimer / 0.2f, 0.0f, 1.0f);
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 255, 80, 80, static_cast<Uint8>(180 * alpha));
      SDL_FRect flashRect = {playerTransform.position.x - cameraPosition.x - 2.0f,
                             playerTransform.position.y - cameraPosition.y - 2.0f,
                             playerCollision.width + 4.0f, playerCollision.height + 4.0f};
      SDL_RenderRect(this->renderer, &flashRect);
    }
  }

  SDL_RenderPresent(this->renderer);
}
