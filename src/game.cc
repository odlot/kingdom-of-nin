#include "game.h"
#include "ecs/component/collision_component.h"
#include "ecs/component/equipment_component.h"
#include "ecs/component/graphic_component.h"
#include "ecs/component/health_component.h"
#include "ecs/component/inventory_component.h"
#include "ecs/component/level_component.h"
#include "ecs/component/mana_component.h"
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
} // namespace

int computeAttackPower(const StatsComponent& stats, const EquipmentComponent& equipment,
                       const ItemDatabase& database) {
  int total = stats.baseAttackPower;
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
  if (equipment.equipped.count(def->slot) > 0) {
    inventory.addItem(*item);
    return false;
  }
  equipment.equipped[def->slot] = *item;
  return true;
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

  { // Spawn a few mobs in each spawn region
    const SDL_Color mobColor = {200, 40, 40, 255};
    for (const Region& region : this->map->getRegions()) {
      if (region.type != RegionType::SpawnRegion) {
        continue;
      }
      const std::array<Coordinate, 3> candidates = {
          Coordinate(region.x + 2, region.y + 2),
          Coordinate(region.x + region.width - 3, region.y + 2),
          Coordinate(region.x + 2, region.y + region.height - 3)};
      int spawned = 0;
      for (const Coordinate& tile : candidates) {
        if (!region.contains(tile.x, tile.y)) {
          continue;
        }
        if (!this->map->isWalkable(tile.x, tile.y)) {
          continue;
        }
        Position mobPosition(tile.x * TILE_SIZE, tile.y * TILE_SIZE);
        int mobEntityId = this->registry->createEntity();
        this->registry->registerComponentForEntity<TransformComponent>(
            std::make_unique<TransformComponent>(mobPosition), mobEntityId);
        this->registry->registerComponentForEntity<GraphicComponent>(
            std::make_unique<GraphicComponent>(mobPosition, mobColor), mobEntityId);
        this->registry->registerComponentForEntity<HealthComponent>(
            std::make_unique<HealthComponent>(20, 20), mobEntityId);
        this->mobEntityIds.push_back(mobEntityId);
        ++spawned;
      }
      if (spawned == 0) {
        std::optional<Coordinate> fallback = firstWalkableInRegion(*this->map, region);
        if (fallback.has_value()) {
          Position mobPosition(fallback->x * TILE_SIZE, fallback->y * TILE_SIZE);
          int mobEntityId = this->registry->createEntity();
          this->registry->registerComponentForEntity<TransformComponent>(
              std::make_unique<TransformComponent>(mobPosition), mobEntityId);
          this->registry->registerComponentForEntity<GraphicComponent>(
              std::make_unique<GraphicComponent>(mobPosition, mobColor), mobEntityId);
          this->registry->registerComponentForEntity<HealthComponent>(
              std::make_unique<HealthComponent>(20, 20), mobEntityId);
          this->mobEntityIds.push_back(mobEntityId);
        }
      }
    }
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
  if (attackPressed && !this->wasAttackPressed && this->attackCooldownRemaining <= 0.0f) {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const StatsComponent& stats =
        this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const int attackPower = computeAttackPower(stats, equipment, *this->itemDatabase);

    const Position playerCenter(playerTransform.position.x + (playerCollision.width / 2.0f),
                                playerTransform.position.y + (playerCollision.height / 2.0f));
    const float rangeSquared = ATTACK_RANGE * ATTACK_RANGE;

    for (int mobEntityId : this->mobEntityIds) {
      HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
      if (mobHealth.current <= 0) {
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

  const TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  Position playerCenter(playerTransform.position.x + (playerCollision.width / 2.0f),
                        playerTransform.position.y + (playerCollision.height / 2.0f));
  this->camera->update(playerCenter);
}

void Game::render() {
  SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
  SDL_RenderClear(this->renderer);
  const Position& cameraPosition = this->camera->getPosition();

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

  { // Floating damage text
    this->floatingTextSystem->render(this->renderer, cameraPosition, this->font);
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
                                 attackPower, this->inventoryUi->isStatsVisible());
  }

  SDL_RenderPresent(this->renderer);
}
