#include "game.h"
#include "ecs/component/buff_component.h"
#include "ecs/component/class_component.h"
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
#include "ecs/component/npc_component.h"
#include "ecs/component/projectile_component.h"
#include "ecs/component/quest_log_component.h"
#include "ecs/component/shop_component.h"
#include "ecs/component/pushback_component.h"
#include "ecs/component/skill_bar_component.h"
#include "ecs/component/skill_tree_component.h"
#include "ecs/component/stats_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/system/graphic_system.h"
#include "ecs/system/movement_system.h"
#include "ecs/system/pushback_system.h"
#include "events/events.h"
#include "ui/buff_bar.h"
#include "ui/floating_text_system.h"
#include "ui/inventory.h"
#include "ui/minimap.h"
#include "ui/skill_bar.h"
#include "ui/skill_tree.h"
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
#include <vector>

constexpr std::string WINDOW_TITLE = "Kingdom of Nin";
constexpr int MINIMAP_WIDTH = 160;
constexpr int MINIMAP_HEIGHT = 120;
constexpr int MINIMAP_MARGIN = 12;
constexpr float ATTACK_RANGE = 56.0f;
constexpr float ATTACK_COOLDOWN_SECONDS = 0.3f;
constexpr float AUTO_TARGET_RANGE_MULTIPLIER = 2.0f;
constexpr float LOOT_PICKUP_RANGE = 40.0f;
constexpr float NPC_INTERACT_RANGE = 52.0f;
constexpr float PLAYER_CRIT_CHANCE = 0.12f;
constexpr float PLAYER_CRIT_MULTIPLIER = 1.5f;
constexpr float FACING_TURN_SPEED = 8.0f;
constexpr float PUSHBACK_DISTANCE = static_cast<float>(TILE_SIZE);
constexpr float PUSHBACK_DURATION = 0.2f;
constexpr float PLAYER_KNOCKBACK_IMMUNITY_SECONDS = 2.0f;
constexpr float RESURRECT_RANGE = 28.0f;
constexpr int SHOP_SELL_DIVISOR = 2;
constexpr float SHOP_NOTICE_DURATION = 1.6f;

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

std::optional<Position> regionCenterByName(const Map& map, const std::string& name) {
  const std::vector<Region>& regions = map.getRegions();
  for (const Region& region : regions) {
    if (regionName(region.type) == name) {
      const float centerX = (region.x + (region.width / 2.0f)) * TILE_SIZE;
      const float centerY = (region.y + (region.height / 2.0f)) * TILE_SIZE;
      return Position(centerX, centerY);
    }
  }
  return std::nullopt;
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

const char* className(CharacterClass characterClass) {
  switch (characterClass) {
  case CharacterClass::Warrior:
    return "Warrior";
  case CharacterClass::Mage:
    return "Mage";
  case CharacterClass::Archer:
    return "Archer";
  case CharacterClass::Rogue:
    return "Rogue";
  case CharacterClass::Any:
    break;
  }
  return "Adventurer";
}

float shortestAngleDiff(float from, float to) {
  constexpr float kPi = 3.14159265f;
  float diff = std::fmod(to - from, 2.0f * kPi);
  if (diff > kPi) {
    diff -= 2.0f * kPi;
  } else if (diff < -kPi) {
    diff += 2.0f * kPi;
  }
  return diff;
}

SDL_Color lootColorForItem(const ItemDef* def) {
  if (!def) {
    return SDL_Color{200, 200, 200, 255};
  }
  switch (def->slot) {
  case ItemSlot::Weapon:
    return SDL_Color{240, 200, 80, 255};
  case ItemSlot::Shield:
    return SDL_Color{120, 180, 240, 255};
  case ItemSlot::Shoulders:
  case ItemSlot::Chest:
  case ItemSlot::Pants:
  case ItemSlot::Boots:
  case ItemSlot::Cape:
    return SDL_Color{120, 220, 140, 255};
  }
  return SDL_Color{200, 200, 200, 255};
}

int itemPrice(const ItemDef* def) {
  return def ? def->price : 0;
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

void drawFacingArc(SDL_Renderer* renderer, const Position& center, float radius, float facingX,
                   float facingY, float halfAngle, const Position& cameraPosition,
                   SDL_Color color) {
  float fx = facingX;
  float fy = facingY;
  const float facingLength = std::sqrt((fx * fx) + (fy * fy));
  if (facingLength <= 0.001f) {
    fx = 0.0f;
    fy = 1.0f;
  } else {
    fx /= facingLength;
    fy /= facingLength;
  }
  const float centerAngle = std::atan2(fy, fx);
  constexpr int kSegments = 16;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  float prevX = center.x + radius * std::cos(centerAngle - halfAngle);
  float prevY = center.y + radius * std::sin(centerAngle - halfAngle);
  for (int i = 1; i <= kSegments; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kSegments);
    const float angle = centerAngle - halfAngle + (t * halfAngle * 2.0f);
    const float nextX = center.x + radius * std::cos(angle);
    const float nextY = center.y + radius * std::sin(angle);
    SDL_RenderLine(renderer, prevX - cameraPosition.x, prevY - cameraPosition.y,
                   nextX - cameraPosition.x, nextY - cameraPosition.y);
    prevX = nextX;
    prevY = nextY;
  }
  SDL_RenderLine(renderer, center.x - cameraPosition.x, center.y - cameraPosition.y,
                 center.x + radius * std::cos(centerAngle - halfAngle) - cameraPosition.x,
                 center.y + radius * std::sin(centerAngle - halfAngle) - cameraPosition.y);
  SDL_RenderLine(renderer, center.x - cameraPosition.x, center.y - cameraPosition.y,
                 center.x + radius * std::cos(centerAngle + halfAngle) - cameraPosition.x,
                 center.y + radius * std::sin(centerAngle + halfAngle) - cameraPosition.y);
}

std::vector<std::string> wrapText(TTF_Font* font, const std::string& text, int maxWidth) {
  std::vector<std::string> lines;
  std::string current;
  std::string word;
  auto flushWord = [&]() {
    if (word.empty()) {
      return;
    }
    std::string candidate = current.empty() ? word : current + " " + word;
    int width = 0;
    int height = 0;
    TTF_GetStringSize(font, candidate.c_str(), candidate.size(), &width, &height);
    if (width <= maxWidth) {
      current = std::move(candidate);
      word.clear();
      return;
    }
    if (!current.empty()) {
      lines.push_back(current);
      current.clear();
    }
    TTF_GetStringSize(font, word.c_str(), word.size(), &width, &height);
    if (width <= maxWidth) {
      current = word;
      word.clear();
      return;
    }
    std::string chunk;
    for (char ch : word) {
      std::string next = chunk + ch;
      TTF_GetStringSize(font, next.c_str(), next.size(), &width, &height);
      if (width > maxWidth && !chunk.empty()) {
        lines.push_back(chunk);
        chunk.clear();
      }
      chunk += ch;
    }
    if (!chunk.empty()) {
      current = chunk;
    }
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flushWord();
      if (!current.empty()) {
        lines.push_back(current);
        current.clear();
      } else {
        lines.push_back("");
      }
      continue;
    }
    if (ch == ' ') {
      flushWord();
    } else {
      word += ch;
    }
  }
  flushWord();
  if (!current.empty()) {
    lines.push_back(current);
  }
  if (lines.empty()) {
    lines.push_back("");
  }
  return lines;
}

struct ShopLayout {
  SDL_FRect panel;
  SDL_FRect shopRect;
  SDL_FRect inventoryRect;
  float rowHeight = 16.0f;
  float columnGap = 12.0f;
};

enum class QuestEntryType { Available = 0, TurnIn };

struct QuestEntry {
  QuestEntryType type = QuestEntryType::Available;
  const QuestDef* def = nullptr;
};

ShopLayout shopLayout(int windowWidth, int windowHeight) {
  constexpr float panelWidth = 520.0f;
  constexpr float panelHeight = 200.0f;
  const float x = (windowWidth - panelWidth) / 2.0f;
  const float y = windowHeight - panelHeight - 120.0f;
  ShopLayout layout;
  layout.panel = SDL_FRect{x, y, panelWidth, panelHeight};
  const float listWidth = (panelWidth - (layout.columnGap * 3.0f)) / 2.0f;
  layout.shopRect = SDL_FRect{x + layout.columnGap, y + 36.0f, listWidth,
                              panelHeight - 48.0f};
  layout.inventoryRect = SDL_FRect{x + (layout.columnGap * 2.0f) + listWidth, y + 36.0f,
                                   listWidth, panelHeight - 48.0f};
  return layout;
}

bool isShopNpc(int npcId, const std::vector<int>& shopNpcIds) {
  return std::find(shopNpcIds.begin(), shopNpcIds.end(), npcId) != shopNpcIds.end();
}

bool pointInRect(float x, float y, const SDL_FRect& rect) {
  return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

std::vector<QuestEntry> buildNpcQuestEntries(const std::string& npcName,
                                             const QuestSystem& questSystem,
                                             const QuestDatabase& questDatabase,
                                             const QuestLogComponent& questLog,
                                             const LevelComponent& level) {
  std::vector<QuestEntry> entries;
  const std::vector<const QuestDef*> available =
      questSystem.availableQuests(npcName, level, questLog);
  for (const QuestDef* def : available) {
    entries.push_back(QuestEntry{QuestEntryType::Available, def});
  }
  for (const QuestProgress& progress : questLog.activeQuests) {
    const QuestDef* def = questDatabase.getQuest(progress.questId);
    if (!def || def->turnInNpcName != npcName) {
      continue;
    }
    if (progress.completed && !progress.rewardsClaimed) {
      entries.push_back(QuestEntry{QuestEntryType::TurnIn, def});
    }
  }
  return entries;
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

bool isInFacingArc(const Position& origin, const Position& target, float facingX, float facingY,
                   float halfAngle) {
  const float dx = target.x - origin.x;
  const float dy = target.y - origin.y;
  const float length = std::sqrt((dx * dx) + (dy * dy));
  if (length <= 0.001f) {
    return true;
  }
  const float facingLength = std::sqrt((facingX * facingX) + (facingY * facingY));
  if (facingLength <= 0.001f) {
    return true;
  }
  const float nx = dx / length;
  const float ny = dy / length;
  const float fnx = facingX / facingLength;
  const float fny = facingY / facingLength;
  const float dot = (nx * fnx) + (ny * fny);
  return dot >= std::cos(halfAngle);
}

struct AttackProfile {
  float range = ATTACK_RANGE;
  float halfAngle = 0.75f;
  float cooldown = ATTACK_COOLDOWN_SECONDS;
  bool isRanged = false;
  float projectileSpeed = 0.0f;
  float projectileRadius = 0.0f;
  float projectileTrailLength = 0.0f;
  SDL_Color projectileColor = {255, 255, 255, 255};
};

struct ClassDefaults {
  int baseAttackPower = 1;
  int baseArmor = 0;
  int strength = 5;
  int dexterity = 5;
  int intellect = 5;
  int luck = 5;
  int health = 100;
  int mana = 50;
  int weaponId = 1;
  int offhandId = 0;
};

void applyOrRefreshBuff(BuffComponent& buffs, int buffId, const std::string& name, float duration) {
  for (BuffInstance& buff : buffs.buffs) {
    if (buff.id == buffId) {
      buff.remaining = duration;
      buff.duration = duration;
      buff.name = name;
      return;
    }
  }
  BuffInstance instance;
  instance.id = buffId;
  instance.name = name;
  instance.remaining = duration;
  instance.duration = duration;
  buffs.buffs.push_back(std::move(instance));
}

bool isSkillUnlocked(const SkillTreeComponent& tree, int skillId) {
  return tree.unlockedSkills.count(skillId) > 0;
}

AttackProfile attackProfileForWeapon(const EquipmentComponent& equipment,
                                     const ItemDatabase& database) {
  AttackProfile profile;
  auto weaponIt = equipment.equipped.find(ItemSlot::Weapon);
  if (weaponIt == equipment.equipped.end()) {
    return profile;
  }
  const ItemDef* def = database.getItem(weaponIt->second.itemId);
  if (!def) {
    return profile;
  }
  switch (def->weaponType) {
  case WeaponType::OneHandedSword:
    profile.halfAngle = 0.75f;
    profile.range = ATTACK_RANGE;
    profile.cooldown = 0.5f;
    break;
  case WeaponType::TwoHandedSword:
    profile.halfAngle = 0.9f;
    profile.range = ATTACK_RANGE * 1.1f;
    profile.cooldown = 0.8f;
    break;
  case WeaponType::Polearm:
    profile.halfAngle = 0.6f;
    profile.range = ATTACK_RANGE * 1.3f;
    profile.cooldown = 0.9f;
    break;
  case WeaponType::Spear:
    profile.halfAngle = 0.5f;
    profile.range = ATTACK_RANGE * 1.4f;
    profile.cooldown = 0.7f;
    break;
  case WeaponType::Bow:
    profile.halfAngle = 0.35f;
    profile.range = ATTACK_RANGE * 3.6f;
    profile.cooldown = 0.9f;
    profile.isRanged = true;
    profile.projectileSpeed = 220.0f;
    profile.projectileRadius = 9.0f;
    profile.projectileTrailLength = 22.0f;
    profile.projectileColor = SDL_Color{255, 220, 120, 255};
    break;
  case WeaponType::Wand:
    profile.halfAngle = 0.4f;
    profile.range = ATTACK_RANGE * 3.0f;
    profile.cooldown = 0.8f;
    profile.isRanged = true;
    profile.projectileSpeed = 200.0f;
    profile.projectileRadius = 10.0f;
    profile.projectileTrailLength = 26.0f;
    profile.projectileColor = SDL_Color{140, 200, 255, 255};
    break;
  case WeaponType::Dagger:
    profile.halfAngle = 0.85f;
    profile.range = ATTACK_RANGE * 0.85f;
    profile.cooldown = 0.35f;
    break;
  case WeaponType::None:
    profile.halfAngle = 0.65f;
    profile.range = ATTACK_RANGE * 0.9f;
    profile.cooldown = ATTACK_COOLDOWN_SECONDS;
    break;
  }
  if (def->projectile.speed > 0.0f) {
    profile.projectileSpeed = def->projectile.speed;
  }
  if (def->projectile.radius > 0.0f) {
    profile.projectileRadius = def->projectile.radius;
  }
  if (def->projectile.trailLength > 0.0f) {
    profile.projectileTrailLength = def->projectile.trailLength;
  }
  return profile;
}

ClassDefaults classDefaultsFor(CharacterClass characterClass) {
  switch (characterClass) {
  case CharacterClass::Warrior:
    return ClassDefaults{2, 1, 8, 4, 2, 3, 130, 30, 1, 2};
  case CharacterClass::Mage:
    return ClassDefaults{1, 0, 2, 4, 9, 4, 80, 90, 7, 0};
  case CharacterClass::Archer:
    return ClassDefaults{1, 0, 4, 9, 3, 4, 95, 50, 6, 0};
  case CharacterClass::Rogue:
    return ClassDefaults{1, 0, 5, 8, 2, 7, 90, 40, 8, 0};
  case CharacterClass::Any:
    return ClassDefaults{1, 0, 5, 5, 5, 5, 100, 50, 1, 2};
  }
  return ClassDefaults{};
}

void applyLevelUps(LevelComponent& level, StatsComponent& stats, SkillTreeComponent& skillTree) {
  while (level.experience >= level.nextLevelExperience) {
    level.experience -= level.nextLevelExperience;
    level.level += 1;
    level.nextLevelExperience += 100;
    stats.unspentPoints += 5;
    skillTree.unspentPoints += 1;
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

void applyPushback(Registry& registry, int targetEntityId, const Position& fromPosition,
                   float distance, float duration) {
  TransformComponent& targetTransform = registry.getComponent<TransformComponent>(targetEntityId);
  const CollisionComponent& targetCollision =
      registry.getComponent<CollisionComponent>(targetEntityId);
  const Position targetCenter = centerForEntity(targetTransform, targetCollision);
  float dx = targetCenter.x - fromPosition.x;
  float dy = targetCenter.y - fromPosition.y;
  const float length = std::sqrt((dx * dx) + (dy * dy));
  if (length <= 0.001f || duration <= 0.0f) {
    return;
  }
  dx /= length;
  dy /= length;

  PushbackComponent& pushback = registry.getComponent<PushbackComponent>(targetEntityId);
  const float speed = distance / duration;
  pushback.velocityX = dx * speed;
  pushback.velocityY = dy * speed;
  pushback.remaining = duration;
}

bool createLootEntity(Registry& registry, const ItemDatabase& database, const Position& position,
                      int itemId, std::vector<int>& lootEntityIds) {
  const ItemDef* def = database.getItem(itemId);
  const SDL_Color lootColor = lootColorForItem(def);
  int entityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), entityId);
  registry.registerComponentForEntity<GraphicComponent>(
      std::make_unique<GraphicComponent>(position, lootColor), entityId);
  registry.registerComponentForEntity<CollisionComponent>(
      std::make_unique<CollisionComponent>(32.0f, 32.0f, false), entityId);
  registry.registerComponentForEntity<LootComponent>(std::make_unique<LootComponent>(itemId),
                                                     entityId);
  lootEntityIds.push_back(entityId);
  return true;
}

int createNpcEntity(Registry& registry, const Position& position, std::string name,
                    std::string dialogLine, std::vector<int>& npcEntityIds) {
  int entityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), entityId);
  registry.registerComponentForEntity<GraphicComponent>(
      std::make_unique<GraphicComponent>(position, SDL_Color({220, 200, 120, 255})), entityId);
  registry.registerComponentForEntity<CollisionComponent>(
      std::make_unique<CollisionComponent>(32.0f, 32.0f, false), entityId);
  registry.registerComponentForEntity<NpcComponent>(
      std::make_unique<NpcComponent>(std::move(name), std::move(dialogLine)), entityId);
  npcEntityIds.push_back(entityId);
  return entityId;
}

int createShopNpcEntity(Registry& registry, const Position& position, std::string name,
                        std::string dialogLine, std::vector<int> stock,
                        std::vector<int>& npcEntityIds, std::vector<int>& shopNpcIds) {
  int entityId = createNpcEntity(registry, position, std::move(name), std::move(dialogLine),
                                 npcEntityIds);
  registry.registerComponentForEntity<ShopComponent>(
      std::make_unique<ShopComponent>(std::move(stock)), entityId);
  shopNpcIds.push_back(entityId);
  return entityId;
}

int createProjectileEntity(Registry& registry, const Position& position, float velocityX,
                           float velocityY, float range, int sourceEntityId, int targetEntityId,
                           int damage, bool isCrit, float radius, float trailLength,
                           SDL_Color color,
                           std::vector<int>& projectileEntityIds) {
  int entityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), entityId);
  auto projectile = std::make_unique<ProjectileComponent>(
      sourceEntityId, targetEntityId, velocityX, velocityY, range, damage, isCrit, radius,
      trailLength, color);
  projectile->lastX = position.x;
  projectile->lastY = position.y;
  registry.registerComponentForEntity<ProjectileComponent>(std::move(projectile), entityId);
  projectileEntityIds.push_back(entityId);
  return entityId;
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
  this->rng = std::mt19937(std::random_device{}());
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
  this->skillDatabase = std::make_unique<SkillDatabase>();
  this->skillTreeDefinition = std::make_unique<SkillTreeDefinition>();
  this->questDatabase = std::make_unique<QuestDatabase>();
  this->questSystem = std::make_unique<QuestSystem>(*this->questDatabase);
  this->eventBus = std::make_unique<EventBus>();
  this->floatingTextSystem = std::make_unique<FloatingTextSystem>(*this->eventBus);
  this->respawnSystem = std::make_unique<RespawnSystem>();
  this->inventoryUi = std::make_unique<Inventory>();
  this->characterStats = std::make_unique<CharacterStats>();
  this->skillBar = std::make_unique<SkillBar>();
  this->buffBar = std::make_unique<BuffBar>();
  this->skillTree = std::make_unique<SkillTree>();
  this->questLogUi = std::make_unique<QuestLog>();
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
  this->registry->registerComponentForEntity<PushbackComponent>(
      std::make_unique<PushbackComponent>(0.0f, 0.0f, 0.0f), this->playerEntityId);
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
  this->registry->registerComponentForEntity<SkillBarComponent>(
      std::make_unique<SkillBarComponent>(), this->playerEntityId);
  this->registry->registerComponentForEntity<SkillTreeComponent>(
      std::make_unique<SkillTreeComponent>(), this->playerEntityId);
  this->registry->registerComponentForEntity<BuffComponent>(std::make_unique<BuffComponent>(),
                                                            this->playerEntityId);
  this->registry->registerComponentForEntity<QuestLogComponent>(
      std::make_unique<QuestLogComponent>(), this->playerEntityId);
  constexpr CharacterClass kDefaultClass = CharacterClass::Any;
  this->registry->registerComponentForEntity<ClassComponent>(
      std::make_unique<ClassComponent>(kDefaultClass), this->playerEntityId);

  {
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    HealthComponent& health = this->registry->getComponent<HealthComponent>(this->playerEntityId);
    ManaComponent& mana = this->registry->getComponent<ManaComponent>(this->playerEntityId);
    const ClassComponent& playerClass =
        this->registry->getComponent<ClassComponent>(this->playerEntityId);
    const ClassDefaults defaults = classDefaultsFor(playerClass.characterClass);
    stats.baseAttackPower = defaults.baseAttackPower;
    stats.baseArmor = defaults.baseArmor;
    stats.strength = defaults.strength;
    stats.dexterity = defaults.dexterity;
    stats.intellect = defaults.intellect;
    stats.luck = defaults.luck;
    health.max = defaults.health;
    health.current = defaults.health;
    mana.max = defaults.mana;
    mana.current = defaults.mana;
    if (defaults.weaponId > 0) {
      ItemInstance weaponInstance{defaults.weaponId};
      if (inventory.addItem(weaponInstance)) {
        equipItemByIndex(inventory, equipment, *this->itemDatabase, 0);
      }
    }
    if (defaults.offhandId > 0) {
      ItemInstance offhandInstance{defaults.offhandId};
      if (inventory.addItem(offhandInstance)) {
        equipItemByIndex(inventory, equipment, *this->itemDatabase, 0);
      }
    }
  }

  { // Assign starter skills to the player
    SkillBarComponent& skills =
        this->registry->getComponent<SkillBarComponent>(this->playerEntityId);
    skills.slots[0].skillId = 1;
    skills.slots[1].skillId = 2;
    skills.slots[2].skillId = 3;
  }

  {
    QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    const LevelComponent& level =
        this->registry->getComponent<LevelComponent>(this->playerEntityId);
    this->questSystem->addQuest(questLog, level, 1);
  }

  {
    int npcTileX = start.x + 1;
    int npcTileY = start.y;
    if (!this->map->isWalkable(npcTileX, npcTileY)) {
      npcTileX = start.x;
      npcTileY = start.y + 1;
    }
    if (!this->map->isWalkable(npcTileX, npcTileY)) {
      npcTileX = start.x;
      npcTileY = start.y;
    }
    Position npcPosition(npcTileX * TILE_SIZE, npcTileY * TILE_SIZE);
    createNpcEntity(*this->registry, npcPosition, "Town Guide",
                    "Welcome! Explore the world and grow strong.", this->npcEntityIds);
  }

  {
    int npcTileX = start.x + 2;
    int npcTileY = start.y;
    if (!this->map->isWalkable(npcTileX, npcTileY)) {
      npcTileX = start.x + 1;
      npcTileY = start.y + 1;
    }
    if (!this->map->isWalkable(npcTileX, npcTileY)) {
      npcTileX = start.x;
      npcTileY = start.y + 2;
    }
    if (this->map->isWalkable(npcTileX, npcTileY)) {
      Position npcPosition(npcTileX * TILE_SIZE, npcTileY * TILE_SIZE);
      createShopNpcEntity(*this->registry, npcPosition, "Shopkeeper",
                          "Need supplies? Take a look.", {1, 5, 6, 7, 8, 2, 3, 4},
                          this->npcEntityIds, this->shopNpcIds);
    }
  }

  { // Unlock the starter skill
    SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    skillTree.unlockedSkills.insert(1);
  }

  { // Spawn loot items near the starting zone
    const std::array<int, 7> lootItems = {1, 2, 3, 4, 5, 6, 7};
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
      createLootEntity(*this->registry, *this->itemDatabase, lootPosition, lootItems[lootIndex],
                       this->lootEntityIds);
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
    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
      this->mouseWheelDelta += event.wheel.y;
    }
  }
  return running;
}

void Game::update(float dt) {
  // Update game logic here
  if (this->attackCooldownRemaining > 0.0f) {
    this->attackCooldownRemaining = std::max(0.0f, this->attackCooldownRemaining - dt);
  }
  if (this->playerKnockbackImmunityRemaining > 0.0f) {
    this->playerKnockbackImmunityRemaining =
        std::max(0.0f, this->playerKnockbackImmunityRemaining - dt);
  }
  this->floatingTextSystem->update(dt);
  this->shopNoticeTimer = std::max(0.0f, this->shopNoticeTimer - dt);
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

  const bool pickupPressed = keyboardState[SDL_SCANCODE_F];
  const bool interactPressed = keyboardState[SDL_SCANCODE_T];
  const bool debugPressed = keyboardState[SDL_SCANCODE_D];
  const bool resurrectPressed = keyboardState[SDL_SCANCODE_R];
  const bool questLogPressed = keyboardState[SDL_SCANCODE_Q];
  const bool acceptQuestPressed = keyboardState[SDL_SCANCODE_A];
  const bool turnInQuestPressed = keyboardState[SDL_SCANCODE_C];
  const bool questPrevPressed = keyboardState[SDL_SCANCODE_PAGEUP];
  const bool questNextPressed = keyboardState[SDL_SCANCODE_PAGEDOWN];
  const std::array<SDL_Scancode, 5> skillKeys = {SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
                                                 SDL_SCANCODE_4, SDL_SCANCODE_5};
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
    SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    this->skillTree->handleInput(keyboardState, static_cast<int>(mouseX),
                                 static_cast<int>(mouseY), mousePressed, skillTree,
                                 *this->skillTreeDefinition, WINDOW_WIDTH);
  }

  {
    SkillBarComponent& skills =
        this->registry->getComponent<SkillBarComponent>(this->playerEntityId);
    BuffComponent& buffs = this->registry->getComponent<BuffComponent>(this->playerEntityId);
    SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);
    for (auto it = buffs.buffs.begin(); it != buffs.buffs.end();) {
      it->remaining = std::max(0.0f, it->remaining - dt);
      if (it->remaining <= 0.0f) {
        it = buffs.buffs.erase(it);
      } else {
        ++it;
      }
    }
    for (std::size_t i = 0; i < skills.slots.size(); ++i) {
      SkillSlot& slot = skills.slots[i];
      if (slot.cooldownRemaining > 0.0f) {
        slot.cooldownRemaining = std::max(0.0f, slot.cooldownRemaining - dt);
      }
      const bool pressed = keyboardState[skillKeys[i]];
      if (pressed && !this->wasSkillPressed[i]) {
        const SkillDef* def =
            (slot.skillId > 0) ? this->skillDatabase->getSkill(slot.skillId) : nullptr;
        const bool unlocked = def && isSkillUnlocked(skillTree, def->id);
        if (!this->isPlayerGhost && unlocked && slot.cooldownRemaining <= 0.0f) {
          slot.cooldownRemaining = def->cooldown;
        this->eventBus->emitFloatingTextEvent(
            FloatingTextEvent{"Skill: " + def->name, playerCenter, FloatingTextKind::Info});
          if (def->buffDuration > 0.0f) {
            applyOrRefreshBuff(buffs, def->id, def->name, def->buffDuration);
          }
        }
      }
      this->wasSkillPressed[i] = pressed;
    }
  }
  if (!this->isPlayerGhost && pickupPressed && !this->wasPickupPressed) {
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
        this->eventBus->emitItemPickupEvent(ItemPickupEvent{item.itemId, 1});
        this->eventBus->emitFloatingTextEvent(
            FloatingTextEvent{"Picked up " + name, playerCenter, FloatingTextKind::Info});
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
  if (questLogPressed && !this->wasQuestLogPressed) {
    this->questLogVisible = !this->questLogVisible;
  }
  this->wasQuestLogPressed = questLogPressed;
  const bool acceptQuestJustPressed = acceptQuestPressed && !this->wasAcceptQuestPressed;
  const bool turnInQuestJustPressed = turnInQuestPressed && !this->wasTurnInQuestPressed;
  const bool questPrevJustPressed = questPrevPressed && !this->wasQuestPrevPressed;
  const bool questNextJustPressed = questNextPressed && !this->wasQuestNextPressed;
  this->wasAcceptQuestPressed = acceptQuestPressed;
  this->wasTurnInQuestPressed = turnInQuestPressed;
  this->wasQuestPrevPressed = questPrevPressed;
  this->wasQuestNextPressed = questNextPressed;
  const bool interactJustPressed = interactPressed && !this->wasInteractPressed;
  this->wasInteractPressed = interactPressed;
  if (debugPressed && !this->wasDebugPressed) {
    this->showDebugMobRanges = !this->showDebugMobRanges;
  }
  this->wasDebugPressed = debugPressed;

  {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);
    const float rangeSquared = NPC_INTERACT_RANGE * NPC_INTERACT_RANGE;
    int closestNpcId = -1;
    float closestDist = rangeSquared;
    for (int npcEntityId : this->npcEntityIds) {
      const TransformComponent& npcTransform =
          this->registry->getComponent<TransformComponent>(npcEntityId);
      const CollisionComponent& npcCollision =
          this->registry->getComponent<CollisionComponent>(npcEntityId);
      const Position npcCenter = centerForEntity(npcTransform, npcCollision);
      const float dist = squaredDistance(playerCenter, npcCenter);
      if (dist <= closestDist) {
        closestDist = dist;
        closestNpcId = npcEntityId;
      }
    }
    this->currentNpcId = closestNpcId;
    if (this->activeNpcId != -1) {
      const TransformComponent& activeTransform =
          this->registry->getComponent<TransformComponent>(this->activeNpcId);
      const CollisionComponent& activeCollision =
          this->registry->getComponent<CollisionComponent>(this->activeNpcId);
      const Position activeCenter = centerForEntity(activeTransform, activeCollision);
      if (squaredDistance(playerCenter, activeCenter) > rangeSquared) {
        this->activeNpcId = -1;
        this->shopOpen = false;
      }
    }
    if (interactJustPressed) {
      if (this->activeNpcId != -1) {
        this->activeNpcId = -1;
        this->shopOpen = false;
      } else if (this->currentNpcId != -1) {
        this->activeNpcId = this->currentNpcId;
        this->npcDialogScroll = 0.0f;
        this->shopOpen = isShopNpc(this->activeNpcId, this->shopNpcIds);
        this->activeNpcQuestSelection = 0;
        QuestLogComponent& questLog =
            this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
        StatsComponent& stats =
            this->registry->getComponent<StatsComponent>(this->playerEntityId);
        LevelComponent& level =
            this->registry->getComponent<LevelComponent>(this->playerEntityId);
        InventoryComponent& inventory =
            this->registry->getComponent<InventoryComponent>(this->playerEntityId);
        const NpcComponent& npc =
            this->registry->getComponent<NpcComponent>(this->activeNpcId);
        const std::vector<std::string> turnedIn =
            this->questSystem->tryTurnIn(npc.name, questLog, stats, level, inventory);
        if (!turnedIn.empty()) {
          const TransformComponent& playerTransform =
              this->registry->getComponent<TransformComponent>(this->playerEntityId);
          const CollisionComponent& playerCollision =
              this->registry->getComponent<CollisionComponent>(this->playerEntityId);
          const Position playerCenter = centerForEntity(playerTransform, playerCollision);
          for (const std::string& questName : turnedIn) {
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{"Completed: " + questName, playerCenter,
                                  FloatingTextKind::Info});
          }
        }
      }
    }
    if (this->activeNpcId != -1 && !this->shopOpen && acceptQuestJustPressed) {
      QuestLogComponent& questLog =
          this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
      LevelComponent& level =
          this->registry->getComponent<LevelComponent>(this->playerEntityId);
      const NpcComponent& npc =
          this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const std::vector<QuestEntry> entries = buildNpcQuestEntries(
          npc.name, *this->questSystem, *this->questDatabase, questLog, level);
      if (!entries.empty() &&
          this->activeNpcQuestSelection >= 0 &&
          this->activeNpcQuestSelection < static_cast<int>(entries.size()) &&
          entries[this->activeNpcQuestSelection].type == QuestEntryType::Available) {
        const QuestDef* def = entries[this->activeNpcQuestSelection].def;
        if (def) {
          this->questSystem->addQuest(questLog, level, def->id);
          const TransformComponent& playerTransform =
              this->registry->getComponent<TransformComponent>(this->playerEntityId);
          const CollisionComponent& playerCollision =
              this->registry->getComponent<CollisionComponent>(this->playerEntityId);
          const Position playerCenter = centerForEntity(playerTransform, playerCollision);
          this->eventBus->emitFloatingTextEvent(
            FloatingTextEvent{"Accepted: " + def->name, playerCenter,
                              FloatingTextKind::Info});
        }
      }
    }
    if (this->activeNpcId != -1 && !this->shopOpen && turnInQuestJustPressed) {
      QuestLogComponent& questLog =
          this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
      StatsComponent& stats =
          this->registry->getComponent<StatsComponent>(this->playerEntityId);
      LevelComponent& level =
          this->registry->getComponent<LevelComponent>(this->playerEntityId);
      InventoryComponent& inventory =
          this->registry->getComponent<InventoryComponent>(this->playerEntityId);
      const NpcComponent& npc =
          this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const std::vector<QuestEntry> entries = buildNpcQuestEntries(
          npc.name, *this->questSystem, *this->questDatabase, questLog, level);
      if (!entries.empty() &&
          this->activeNpcQuestSelection >= 0 &&
          this->activeNpcQuestSelection < static_cast<int>(entries.size()) &&
          entries[this->activeNpcQuestSelection].type == QuestEntryType::TurnIn) {
        const QuestDef* def = entries[this->activeNpcQuestSelection].def;
        if (def) {
          const std::vector<std::string> turnedIn =
              this->questSystem->tryTurnIn(npc.name, questLog, stats, level, inventory);
      if (!turnedIn.empty()) {
        const TransformComponent& playerTransform =
            this->registry->getComponent<TransformComponent>(this->playerEntityId);
        const CollisionComponent& playerCollision =
            this->registry->getComponent<CollisionComponent>(this->playerEntityId);
        const Position playerCenter = centerForEntity(playerTransform, playerCollision);
        for (const std::string& questName : turnedIn) {
          this->eventBus->emitFloatingTextEvent(
              FloatingTextEvent{"Completed: " + questName, playerCenter,
                                FloatingTextKind::Info});
        }
      }
        }
      }
    }
    if (this->activeNpcId != -1 && !this->shopOpen &&
        (questPrevJustPressed || questNextJustPressed)) {
      QuestLogComponent& questLog =
          this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
      LevelComponent& level =
          this->registry->getComponent<LevelComponent>(this->playerEntityId);
      const NpcComponent& npc =
          this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const std::vector<QuestEntry> entries = buildNpcQuestEntries(
          npc.name, *this->questSystem, *this->questDatabase, questLog, level);
      if (!entries.empty()) {
        const int count = static_cast<int>(entries.size());
        if (questNextJustPressed) {
          this->activeNpcQuestSelection = (this->activeNpcQuestSelection + 1) % count;
        } else if (questPrevJustPressed) {
          this->activeNpcQuestSelection =
              (this->activeNpcQuestSelection - 1 + count) % count;
        }
      } else {
        this->activeNpcQuestSelection = 0;
      }
    }
    if (this->activeNpcId != -1 && !this->shopOpen && this->mouseWheelDelta != 0.0f) {
      this->npcDialogScroll -= this->mouseWheelDelta * 18.0f;
    }
  }
  const bool click = mousePressed && !this->wasMousePressed;
  this->wasMousePressed = mousePressed;
  if (this->shopOpen && this->activeNpcId != -1) {
    ShopComponent& shop = this->registry->getComponent<ShopComponent>(this->activeNpcId);
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const ShopLayout layout = shopLayout(WINDOW_WIDTH, WINDOW_HEIGHT);
    const int shopVisibleRows =
        std::max(1, static_cast<int>(std::floor(layout.shopRect.h / layout.rowHeight)));
    const int invVisibleRows =
        std::max(1, static_cast<int>(std::floor(layout.inventoryRect.h / layout.rowHeight)));
    const int shopMaxScrollRows =
        std::max(0, static_cast<int>(shop.stock.size()) - shopVisibleRows);
    const int invMaxScrollRows =
        std::max(0, static_cast<int>(inventory.items.size()) - invVisibleRows);

    if (this->mouseWheelDelta != 0.0f) {
      if (pointInRect(mouseX, mouseY, layout.shopRect)) {
        this->shopScroll -= this->mouseWheelDelta * layout.rowHeight;
      } else if (pointInRect(mouseX, mouseY, layout.inventoryRect)) {
        this->inventoryScroll -= this->mouseWheelDelta * layout.rowHeight;
      } else {
        this->shopScroll -= this->mouseWheelDelta * layout.rowHeight;
        this->inventoryScroll -= this->mouseWheelDelta * layout.rowHeight;
      }
    }
    this->shopScroll = std::clamp(this->shopScroll, 0.0f,
                                  static_cast<float>(shopMaxScrollRows) * layout.rowHeight);
    this->inventoryScroll = std::clamp(
        this->inventoryScroll, 0.0f,
        static_cast<float>(invMaxScrollRows) * layout.rowHeight);
    const int shopStartIndex = static_cast<int>(std::floor(this->shopScroll / layout.rowHeight));
    const int invStartIndex =
        static_cast<int>(std::floor(this->inventoryScroll / layout.rowHeight));

    if (click && pointInRect(mouseX, mouseY, layout.shopRect)) {
      const int row =
          static_cast<int>(std::floor((mouseY - layout.shopRect.y) / layout.rowHeight));
      const int index = shopStartIndex + row;
      if (row >= 0 && row < shopVisibleRows && index >= 0 &&
          index < static_cast<int>(shop.stock.size())) {
        const int itemId = shop.stock[index];
        const ItemDef* def = this->itemDatabase->getItem(itemId);
        const int price = itemPrice(def);
        if (def && price > 0) {
          if (stats.gold < price) {
            this->shopNotice = "Not enough gold.";
            this->shopNoticeTimer = SHOP_NOTICE_DURATION;
          } else if (!inventory.addItem(ItemInstance{itemId})) {
            this->shopNotice = "Inventory full.";
            this->shopNoticeTimer = SHOP_NOTICE_DURATION;
          } else {
            stats.gold -= price;
            this->shopNotice = "Purchased " + def->name + ".";
            this->shopNoticeTimer = SHOP_NOTICE_DURATION;
          }
        }
      }
    }

    if (click && pointInRect(mouseX, mouseY, layout.inventoryRect)) {
      const int row =
          static_cast<int>(std::floor((mouseY - layout.inventoryRect.y) / layout.rowHeight));
      const int index = invStartIndex + row;
      if (row >= 0 && row < invVisibleRows && index >= 0 &&
          index < static_cast<int>(inventory.items.size())) {
        ItemInstance item = inventory.items[index];
        const ItemDef* def = this->itemDatabase->getItem(item.itemId);
        const int basePrice = itemPrice(def);
        const int sellPrice =
            basePrice > 0 ? std::max(1, basePrice / SHOP_SELL_DIVISOR) : 0;
        if (inventory.removeItemAt(static_cast<std::size_t>(index))) {
          stats.gold += sellPrice;
          if (def) {
            this->shopNotice = "Sold " + def->name + ".";
          } else {
            this->shopNotice = "Sold item.";
          }
          this->shopNoticeTimer = SHOP_NOTICE_DURATION;
        }
      }
    }
  }
  if (this->questLogVisible && !this->shopOpen && this->mouseWheelDelta != 0.0f) {
    const SDL_FRect panel = this->questLogUi->panelRect(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (pointInRect(mouseX, mouseY, panel)) {
      this->questLogScroll -= this->mouseWheelDelta * 18.0f;
    }
  }
  this->mouseWheelDelta = 0.0f;

  {
    if (this->isPlayerGhost) {
      this->currentAutoTargetId = -1;
    } else {
      const TransformComponent& playerTransform =
          this->registry->getComponent<TransformComponent>(this->playerEntityId);
      const CollisionComponent& playerCollision =
          this->registry->getComponent<CollisionComponent>(this->playerEntityId);
      const EquipmentComponent& equipment =
          this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
      const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
      const Position playerCenter = centerForEntity(playerTransform, playerCollision);
      const float range = attackProfile.range * AUTO_TARGET_RANGE_MULTIPLIER;
      const float rangeSquared = range * range;
      int closestMobId = -1;
      float closestDist = rangeSquared;
      Position closestCenter;
      for (int mobEntityId : this->mobEntityIds) {
        const HealthComponent& mobHealth =
            this->registry->getComponent<HealthComponent>(mobEntityId);
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
        const float dist = squaredDistance(playerCenter, mobCenter);
        if (dist <= closestDist) {
          closestDist = dist;
          closestMobId = mobEntityId;
          closestCenter = mobCenter;
        }
      }

      this->currentAutoTargetId = closestMobId;
      float desiredAngle = this->facingAngle;
      bool hasFacingTarget = false;
      if (closestMobId != -1) {
        desiredAngle = std::atan2(closestCenter.y - playerCenter.y,
                                  closestCenter.x - playerCenter.x);
        hasFacingTarget = true;
      } else if (x != 0 || y != 0) {
        desiredAngle = std::atan2(static_cast<float>(y), static_cast<float>(x));
        hasFacingTarget = true;
      }
      if (hasFacingTarget) {
        const float diff = shortestAngleDiff(this->facingAngle, desiredAngle);
        const float maxStep = FACING_TURN_SPEED * dt;
        if (std::abs(diff) <= maxStep) {
          this->facingAngle = desiredAngle;
        } else {
          this->facingAngle += (diff > 0.0f ? maxStep : -maxStep);
        }
        this->facingX = std::cos(this->facingAngle);
        this->facingY = std::sin(this->facingAngle);
      }
    }
  }

  auto applyPlayerDamageToMob = [&](int mobEntityId, int damage, bool isCrit,
                                    const Position& hitPosition, const Position& fromPosition) {
    if (mobEntityId == -1) {
      return;
    }
    HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
    if (mobHealth.current <= 0 || this->respawnSystem->isSpawning(mobEntityId)) {
      return;
    }
    mobHealth.current = std::max(0, mobHealth.current - damage);
    applyPushback(*this->registry, mobEntityId, fromPosition, PUSHBACK_DISTANCE, PUSHBACK_DURATION);
    this->eventBus->emitDamageEvent(
        DamageEvent{this->playerEntityId, mobEntityId, damage, hitPosition});
    this->eventBus->emitFloatingTextEvent(
        FloatingTextEvent{std::to_string(damage), hitPosition,
                          isCrit ? FloatingTextKind::CritDamage : FloatingTextKind::Damage});
    if (mobHealth.current == 0) {
      LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
      const MobComponent& mob = this->registry->getComponent<MobComponent>(mobEntityId);
      this->eventBus->emitMobKilledEvent(MobKilledEvent{mob.type, mobEntityId});
      level.experience += mob.experience;
      this->eventBus->emitFloatingTextEvent(
          FloatingTextEvent{"XP +" + std::to_string(mob.experience), hitPosition,
                            FloatingTextKind::Info});
      GraphicComponent& mobGraphic = this->registry->getComponent<GraphicComponent>(mobEntityId);
      mobGraphic.color = SDL_Color({80, 80, 80, 255});
    }
  };

  {
    for (std::size_t i = 0; i < this->projectileEntityIds.size();) {
      const int projectileId = this->projectileEntityIds[i];
      TransformComponent& projectileTransform =
          this->registry->getComponent<TransformComponent>(projectileId);
      ProjectileComponent& projectile =
          this->registry->getComponent<ProjectileComponent>(projectileId);
      bool shouldRemove = false;

      projectile.lastX = projectileTransform.position.x;
      projectile.lastY = projectileTransform.position.y;
      const float dx = projectile.velocityX * dt;
      const float dy = projectile.velocityY * dt;
      projectileTransform.position.x += dx;
      projectileTransform.position.y += dy;
      projectile.remainingRange =
          std::max(0.0f, projectile.remainingRange - std::sqrt((dx * dx) + (dy * dy)));

      const int tileX = static_cast<int>(projectileTransform.position.x / TILE_SIZE);
      const int tileY = static_cast<int>(projectileTransform.position.y / TILE_SIZE);
      if (!this->map->isWalkable(tileX, tileY) || projectile.remainingRange <= 0.0f) {
        shouldRemove = true;
      }

      if (!shouldRemove && projectile.targetEntityId != -1) {
        const HealthComponent& mobHealth =
            this->registry->getComponent<HealthComponent>(projectile.targetEntityId);
        if (mobHealth.current <= 0 || this->respawnSystem->isSpawning(projectile.targetEntityId)) {
          shouldRemove = true;
        } else {
          const TransformComponent& mobTransform =
              this->registry->getComponent<TransformComponent>(projectile.targetEntityId);
          const CollisionComponent& mobCollision =
              this->registry->getComponent<CollisionComponent>(projectile.targetEntityId);
          const Position mobCenter = centerForEntity(mobTransform, mobCollision);
          const float radius = (mobCollision.width / 2.0f) + projectile.radius;
          if (squaredDistance(projectileTransform.position, mobCenter) <= radius * radius) {
            const TransformComponent& playerTransform =
                this->registry->getComponent<TransformComponent>(this->playerEntityId);
            const CollisionComponent& playerCollision =
                this->registry->getComponent<CollisionComponent>(this->playerEntityId);
            const Position playerCenter = centerForEntity(playerTransform, playerCollision);
            applyPlayerDamageToMob(projectile.targetEntityId, projectile.damage,
                                   projectile.isCrit, mobCenter, playerCenter);
            shouldRemove = true;
          }
        }
      }

      if (shouldRemove) {
        projectileTransform.position = Position(-1000.0f, -1000.0f);
        this->projectileEntityIds.erase(this->projectileEntityIds.begin() + i);
      } else {
        ++i;
      }
    }
  }

  if (!this->isPlayerGhost && this->attackCooldownRemaining <= 0.0f) {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const StatsComponent& stats =
        this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const int attackPower = computeAttackPower(stats, equipment, *this->itemDatabase);
    const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
    std::uniform_real_distribution<float> critRoll(0.0f, 1.0f);
    const bool isCrit = critRoll(this->rng) <= PLAYER_CRIT_CHANCE;
    const int attackDamage = isCrit
                                 ? static_cast<int>(std::round(attackPower * PLAYER_CRIT_MULTIPLIER))
                                 : attackPower;

    if (this->currentAutoTargetId != -1) {
      const TransformComponent& mobTransform =
          this->registry->getComponent<TransformComponent>(this->currentAutoTargetId);
      const Position mobCenter(mobTransform.position.x + (TILE_SIZE / 2.0f),
                               mobTransform.position.y + (TILE_SIZE / 2.0f));
      const Position playerCenter = centerForEntity(playerTransform, playerCollision);
      const float rangeSquared = attackProfile.range * attackProfile.range;
      if (squaredDistance(playerCenter, mobCenter) <= rangeSquared) {
        if (attackProfile.isRanged) {
          float dx = mobCenter.x - playerCenter.x;
          float dy = mobCenter.y - playerCenter.y;
          const float length = std::sqrt((dx * dx) + (dy * dy));
          if (length > 0.001f) {
            dx /= length;
            dy /= length;
            const float spawnOffset =
                (playerCollision.width / 2.0f) + attackProfile.projectileRadius + 4.0f;
            const Position spawnPosition(playerCenter.x + (dx * spawnOffset),
                                         playerCenter.y + (dy * spawnOffset));
            createProjectileEntity(*this->registry, spawnPosition,
                                   dx * attackProfile.projectileSpeed,
                                   dy * attackProfile.projectileSpeed, attackProfile.range,
                                   this->playerEntityId, this->currentAutoTargetId, attackDamage,
                                   isCrit, attackProfile.projectileRadius,
                                   attackProfile.projectileTrailLength,
                                   attackProfile.projectileColor, this->projectileEntityIds);
            this->attackCooldownRemaining = attackProfile.cooldown;
          }
        } else {
          applyPlayerDamageToMob(this->currentAutoTargetId, attackDamage, isCrit, mobCenter,
                                 playerCenter);
          this->attackCooldownRemaining = attackProfile.cooldown;
        }
      }
    }
  }

  // spdlog::get("console")->info("Input direction: ({}, {})", result.first,
  //                               result.second);
  //  TODO: Refactor system iteration based on whether update or render is
  //  needed, implement to an interface?
  for (auto it = this->registry->systemsBegin(); it != this->registry->systemsEnd(); ++it) {
    MovementSystem* movementSystem = dynamic_cast<MovementSystem*>((*it).get());
    if (movementSystem) {
      movementSystem->update(result, dt, *this->map);
    }
    PushbackSystem* pushbackSystem = dynamic_cast<PushbackSystem*>((*it).get());
    if (pushbackSystem) {
      pushbackSystem->update(dt, *this->map);
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
      const HealthComponent& playerHealth =
          this->registry->getComponent<HealthComponent>(this->playerEntityId);
      const bool playerAlive = !this->isPlayerGhost && playerHealth.current > 0;
      const bool playerInRegion =
          playerTileX >= mob.regionX && playerTileX < mob.regionX + mob.regionWidth &&
          playerTileY >= mob.regionY && playerTileY < mob.regionY + mob.regionHeight;
      const float distToPlayer = squaredDistance(mobCenter, playerCenter);

      const Position homePosition(mob.homeX * TILE_SIZE, mob.homeY * TILE_SIZE);
      const Position homeCenter(homePosition.x + (mobCollision.width / 2.0f),
                                homePosition.y + (mobCollision.height / 2.0f));
      const float distToHome = squaredDistance(mobCenter, homeCenter);

      std::optional<Position> target;
      if (playerAlive && playerInRegion && distToPlayer <= (mob.aggroRange * mob.aggroRange)) {
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
        if (playerAlive && squaredDistance(mobCenter, playerCenter) <= attackRangeSquared) {
          HealthComponent& playerHealth =
              this->registry->getComponent<HealthComponent>(this->playerEntityId);
          if (playerHealth.current > 0) {
            playerHealth.current = std::max(0, playerHealth.current - mob.attackDamage);
            this->eventBus->emitDamageEvent(
                DamageEvent{mobEntityId, this->playerEntityId, mob.attackDamage, playerCenter});
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{"-" + std::to_string(mob.attackDamage), playerCenter,
                                  FloatingTextKind::Damage});
            if (this->playerKnockbackImmunityRemaining <= 0.0f) {
              applyPushback(*this->registry, this->playerEntityId, mobCenter, PUSHBACK_DISTANCE,
                            PUSHBACK_DURATION);
              this->playerKnockbackImmunityRemaining = PLAYER_KNOCKBACK_IMMUNITY_SECONDS;
            }
            this->playerHitFlashTimer = 0.2f;
            mob.attackTimer = mob.attackCooldown;
          }
        }
      }
    }
  }

  {
    TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    GraphicComponent& playerGraphic =
        this->registry->getComponent<GraphicComponent>(this->playerEntityId);
    HealthComponent& playerHealth =
        this->registry->getComponent<HealthComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);

    if (!this->isPlayerGhost && playerHealth.current <= 0) {
      this->isPlayerGhost = true;
      this->hasCorpse = true;
      this->corpsePosition = playerTransform.position;
      playerGraphic.color = SDL_Color({160, 200, 255, 180});
      this->eventBus->emitFloatingTextEvent(
          FloatingTextEvent{"You died", playerCenter, FloatingTextKind::Info});
    }

    if (this->isPlayerGhost && this->hasCorpse) {
      const Position corpseCenter(this->corpsePosition.x + (playerCollision.width / 2.0f),
                                  this->corpsePosition.y + (playerCollision.height / 2.0f));
      const float distToCorpse = squaredDistance(playerCenter, corpseCenter);
      if (distToCorpse <= (RESURRECT_RANGE * RESURRECT_RANGE) && resurrectPressed &&
          !this->wasResurrectPressed) {
        this->isPlayerGhost = false;
        this->hasCorpse = false;
        playerHealth.current = playerHealth.max;
        playerGraphic.color = SDL_Color({240, 240, 240, 255});
        this->playerKnockbackImmunityRemaining = 0.0f;
        this->eventBus->emitFloatingTextEvent(
            FloatingTextEvent{"Resurrected", playerCenter, FloatingTextKind::Info});
      }
    }
    this->wasResurrectPressed = resurrectPressed;
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
  {
    QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    this->questSystem->update(*this->eventBus, questLog, stats, level, inventory);
    SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    applyLevelUps(level, stats, skillTree);
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

  { // Projectiles (drawn after entities so they stay visible)
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    for (int projectileId : this->projectileEntityIds) {
      const TransformComponent& projectileTransform =
          this->registry->getComponent<TransformComponent>(projectileId);
      const ProjectileComponent& projectile =
          this->registry->getComponent<ProjectileComponent>(projectileId);
      const float size = projectile.radius * 2.0f;
      SDL_FRect projectileRect = {projectileTransform.position.x - projectile.radius -
                                      cameraPosition.x,
                                  projectileTransform.position.y - projectile.radius -
                                      cameraPosition.y,
                                  size, size};
      SDL_SetRenderDrawColor(this->renderer, projectile.color.r, projectile.color.g,
                             projectile.color.b, projectile.color.a);
      SDL_RenderFillRect(this->renderer, &projectileRect);
      if (projectile.trailLength > 0.0f) {
        const float speed = std::sqrt((projectile.velocityX * projectile.velocityX) +
                                      (projectile.velocityY * projectile.velocityY));
        if (speed > 0.001f) {
          const float nx = projectile.velocityX / speed;
          const float ny = projectile.velocityY / speed;
          const float tailX = projectileTransform.position.x - (nx * projectile.trailLength);
          const float tailY = projectileTransform.position.y - (ny * projectile.trailLength);
          SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 200);
          SDL_RenderLine(this->renderer, tailX - cameraPosition.x, tailY - cameraPosition.y,
                         projectileTransform.position.x - cameraPosition.x,
                         projectileTransform.position.y - cameraPosition.y);
        }
      }
      drawCircle(this->renderer, projectileTransform.position, projectile.radius + 2.0f,
                 cameraPosition, SDL_Color{255, 255, 255, 200});
      SDL_SetRenderDrawColor(this->renderer, 30, 30, 30, 200);
      SDL_RenderRect(this->renderer, &projectileRect);
    }
  }

  { // Quest turn-in markers above NPCs
    const QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    for (const QuestProgress& progress : questLog.activeQuests) {
      const QuestDef* def = this->questDatabase->getQuest(progress.questId);
      if (!def || def->turnInNpcName.empty() || !progress.completed || progress.rewardsClaimed) {
        continue;
      }
      for (int npcId : this->npcEntityIds) {
        const NpcComponent& npc = this->registry->getComponent<NpcComponent>(npcId);
        if (npc.name != def->turnInNpcName) {
          continue;
        }
        const TransformComponent& npcTransform =
            this->registry->getComponent<TransformComponent>(npcId);
        const CollisionComponent& npcCollision =
            this->registry->getComponent<CollisionComponent>(npcId);
        const Position npcCenter = centerForEntity(npcTransform, npcCollision);
        SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
        drawCircle(this->renderer, Position(npcCenter.x, npcCenter.y - 18.0f), 6.0f,
                   cameraPosition, SDL_Color{255, 220, 80, 200});
      }
    }
  }

  if (this->hasCorpse) {
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(this->renderer, 180, 200, 255, 200);
    SDL_FRect corpseRect = {this->corpsePosition.x - cameraPosition.x,
                            this->corpsePosition.y - cameraPosition.y,
                            static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
    SDL_RenderRect(this->renderer, &corpseRect);
    SDL_FRect corpseFill = {corpseRect.x + 6.0f, corpseRect.y + 6.0f, corpseRect.w - 12.0f,
                            corpseRect.h - 12.0f};
    SDL_SetRenderDrawColor(this->renderer, 120, 160, 220, 120);
    SDL_RenderFillRect(this->renderer, &corpseFill);
  }

  { // Player facing marker
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const float centerX = playerTransform.position.x + (playerCollision.width / 2.0f);
    const float centerY = playerTransform.position.y + (playerCollision.height / 2.0f);
    const float markerOffset = (playerCollision.width / 2.0f) + 2.0f;
    const float markerSize = 6.0f;
    const float markerX = centerX + (this->facingX * markerOffset) - (markerSize / 2.0f);
    const float markerY = centerY + (this->facingY * markerOffset) - (markerSize / 2.0f);
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    SDL_FRect markerRect = {markerX - cameraPosition.x, markerY - cameraPosition.y, markerSize,
                            markerSize};
    SDL_RenderFillRect(this->renderer, &markerRect);
  }

  { // Facing arc (melee)
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);
    const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    drawFacingArc(this->renderer, playerCenter, attackProfile.range, this->facingX, this->facingY,
                  attackProfile.halfAngle, cameraPosition, SDL_Color{255, 255, 255, 80});
  }

  { // Auto-attack target ring
    if (this->currentAutoTargetId != -1) {
      const HealthComponent& mobHealth =
          this->registry->getComponent<HealthComponent>(this->currentAutoTargetId);
      if (mobHealth.current > 0 &&
          !this->respawnSystem->isSpawning(this->currentAutoTargetId)) {
        const TransformComponent& mobTransform =
            this->registry->getComponent<TransformComponent>(this->currentAutoTargetId);
        const CollisionComponent& mobCollision =
            this->registry->getComponent<CollisionComponent>(this->currentAutoTargetId);
        const Position mobCenter = centerForEntity(mobTransform, mobCollision);
        SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
        drawCircle(this->renderer, mobCenter, (mobCollision.width / 2.0f) + 6.0f,
                   cameraPosition, SDL_Color{255, 230, 120, 220});
        drawCircle(this->renderer, mobCenter, (mobCollision.width / 2.0f) + 3.0f,
                   cameraPosition, SDL_Color{255, 255, 255, 140});
      }
    }
  }

  { // Loot labels and pickup prompt
    if (!this->isPlayerGhost) {
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

      for (int lootId : this->lootEntityIds) {
        const LootComponent& loot = this->registry->getComponent<LootComponent>(lootId);
        const ItemDef* def = this->itemDatabase->getItem(loot.itemId);
        if (!def) {
          continue;
        }
        const TransformComponent& lootTransform =
            this->registry->getComponent<TransformComponent>(lootId);
        const SDL_Color labelColor = lootColorForItem(def);
        const std::string label = def->name;
        SDL_Surface* surface =
            TTF_RenderText_Solid(this->font, label.c_str(), label.length(), labelColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
        SDL_FRect textRect = {lootTransform.position.x - cameraPosition.x - 4.0f,
                              lootTransform.position.y - cameraPosition.y - 16.0f,
                              static_cast<float>(surface->w), static_cast<float>(surface->h)};
        SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
        SDL_FRect bgRect = {textRect.x - 4.0f, textRect.y - 2.0f, textRect.w + 8.0f,
                            textRect.h + 4.0f};
        SDL_RenderFillRect(this->renderer, &bgRect);
        SDL_RenderTexture(this->renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);

        if (lootId == closestLootId) {
          const std::string prompt = "Press F to pick up " + def->name;
          SDL_Color promptColor = {255, 245, 210, 255};
          SDL_Surface* promptSurface =
              TTF_RenderText_Solid(this->font, prompt.c_str(), prompt.length(), promptColor);
          SDL_Texture* promptTexture =
              SDL_CreateTextureFromSurface(this->renderer, promptSurface);
          SDL_FRect promptRect = {lootTransform.position.x - cameraPosition.x - 6.0f,
                                  lootTransform.position.y - cameraPosition.y + TILE_SIZE + 4.0f,
                                  static_cast<float>(promptSurface->w),
                                  static_cast<float>(promptSurface->h)};
          SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
          SDL_FRect promptBg = {promptRect.x - 4.0f, promptRect.y - 2.0f, promptRect.w + 8.0f,
                                promptRect.h + 4.0f};
          SDL_RenderFillRect(this->renderer, &promptBg);
          SDL_RenderTexture(this->renderer, promptTexture, nullptr, &promptRect);
          SDL_DestroySurface(promptSurface);
          SDL_DestroyTexture(promptTexture);
        }
      }
    }
  }

  { // NPC prompt
    if (this->currentNpcId != -1 && this->activeNpcId == -1) {
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->currentNpcId);
      const std::string prompt = "Press T to talk to " + npc.name;
      SDL_Color promptColor = {240, 230, 200, 255};
      SDL_Surface* promptSurface =
          TTF_RenderText_Solid(this->font, prompt.c_str(), prompt.length(), promptColor);
      SDL_Texture* promptTexture = SDL_CreateTextureFromSurface(this->renderer, promptSurface);
      SDL_FRect promptRect = {12.0f, WINDOW_HEIGHT - 140.0f,
                              static_cast<float>(promptSurface->w),
                              static_cast<float>(promptSurface->h)};
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
      SDL_FRect bgRect = {promptRect.x - 6.0f, promptRect.y - 4.0f, promptRect.w + 12.0f,
                          promptRect.h + 8.0f};
      SDL_RenderFillRect(this->renderer, &bgRect);
      SDL_RenderTexture(this->renderer, promptTexture, nullptr, &promptRect);
      SDL_DestroySurface(promptSurface);
      SDL_DestroyTexture(promptTexture);
    }
  }

  { // Shop UI
    if (this->shopOpen && this->activeNpcId != -1) {
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const ShopComponent& shop = this->registry->getComponent<ShopComponent>(this->activeNpcId);
      const InventoryComponent& inventory =
          this->registry->getComponent<InventoryComponent>(this->playerEntityId);
      const StatsComponent& stats =
          this->registry->getComponent<StatsComponent>(this->playerEntityId);
      const ShopLayout layout = shopLayout(WINDOW_WIDTH, WINDOW_HEIGHT);
      const int shopHoveredRow = pointInRect(mouseX, mouseY, layout.shopRect)
                                     ? static_cast<int>(std::floor((mouseY - layout.shopRect.y) /
                                                                   layout.rowHeight))
                                     : -1;
      const int invHoveredRow = pointInRect(mouseX, mouseY, layout.inventoryRect)
                                    ? static_cast<int>(std::floor((mouseY - layout.inventoryRect.y) /
                                                                  layout.rowHeight))
                                    : -1;
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 12, 12, 12, 220);
      SDL_RenderFillRect(this->renderer, &layout.panel);
      SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 140);
      SDL_RenderRect(this->renderer, &layout.panel);

      SDL_Color titleColor = {255, 240, 210, 255};
      SDL_Color headerColor = {220, 220, 220, 255};
      SDL_Color textColor = {235, 235, 235, 255};
      SDL_Color hintColor = {180, 180, 180, 255};

      const std::string title = npc.name + " - Shop";
      SDL_Surface* titleSurface =
          TTF_RenderText_Solid(this->font, title.c_str(), title.length(), titleColor);
      SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(this->renderer, titleSurface);
      SDL_FRect titleRect = {layout.panel.x + 12.0f, layout.panel.y + 8.0f,
                             static_cast<float>(titleSurface->w),
                             static_cast<float>(titleSurface->h)};
      SDL_RenderTexture(this->renderer, titleTexture, nullptr, &titleRect);
      SDL_DestroySurface(titleSurface);
      SDL_DestroyTexture(titleTexture);

      const std::string goldText = "Gold: " + std::to_string(stats.gold);
      SDL_Surface* goldSurface =
          TTF_RenderText_Solid(this->font, goldText.c_str(), goldText.length(), headerColor);
      SDL_Texture* goldTexture = SDL_CreateTextureFromSurface(this->renderer, goldSurface);
      SDL_FRect goldRect = {layout.panel.x + layout.panel.w - goldSurface->w - 12.0f,
                            layout.panel.y + 8.0f, static_cast<float>(goldSurface->w),
                            static_cast<float>(goldSurface->h)};
      SDL_RenderTexture(this->renderer, goldTexture, nullptr, &goldRect);
      SDL_DestroySurface(goldSurface);
      SDL_DestroyTexture(goldTexture);

      const std::string shopHeader = "Shop";
      SDL_Surface* shopSurface =
          TTF_RenderText_Solid(this->font, shopHeader.c_str(), shopHeader.length(), headerColor);
      SDL_Texture* shopTexture = SDL_CreateTextureFromSurface(this->renderer, shopSurface);
      SDL_FRect shopHeaderRect = {layout.shopRect.x, layout.shopRect.y - 18.0f,
                                  static_cast<float>(shopSurface->w),
                                  static_cast<float>(shopSurface->h)};
      SDL_RenderTexture(this->renderer, shopTexture, nullptr, &shopHeaderRect);
      SDL_DestroySurface(shopSurface);
      SDL_DestroyTexture(shopTexture);

      const std::string invHeader = "Inventory";
      SDL_Surface* invSurface =
          TTF_RenderText_Solid(this->font, invHeader.c_str(), invHeader.length(), headerColor);
      SDL_Texture* invTexture = SDL_CreateTextureFromSurface(this->renderer, invSurface);
      SDL_FRect invHeaderRect = {layout.inventoryRect.x, layout.inventoryRect.y - 18.0f,
                                 static_cast<float>(invSurface->w),
                                 static_cast<float>(invSurface->h)};
      SDL_RenderTexture(this->renderer, invTexture, nullptr, &invHeaderRect);
      SDL_DestroySurface(invSurface);
      SDL_DestroyTexture(invTexture);

      const int shopVisibleRows =
          std::max(1, static_cast<int>(std::floor(layout.shopRect.h / layout.rowHeight)));
      const int invVisibleRows =
          std::max(1, static_cast<int>(std::floor(layout.inventoryRect.h / layout.rowHeight)));
      const int shopMaxScrollRows =
          std::max(0, static_cast<int>(shop.stock.size()) - shopVisibleRows);
      const int invMaxScrollRows =
          std::max(0, static_cast<int>(inventory.items.size()) - invVisibleRows);
      this->shopScroll = std::clamp(this->shopScroll, 0.0f,
                                    static_cast<float>(shopMaxScrollRows) * layout.rowHeight);
      this->inventoryScroll = std::clamp(
          this->inventoryScroll, 0.0f,
          static_cast<float>(invMaxScrollRows) * layout.rowHeight);
      const int shopStartIndex = static_cast<int>(std::floor(this->shopScroll / layout.rowHeight));
      const int invStartIndex =
          static_cast<int>(std::floor(this->inventoryScroll / layout.rowHeight));

      for (int i = 0; i < shopVisibleRows; ++i) {
        const int index = shopStartIndex + i;
        if (index < 0 || index >= static_cast<int>(shop.stock.size())) {
          break;
        }
        const int itemId = shop.stock[index];
        const ItemDef* def = this->itemDatabase->getItem(itemId);
        if (!def) {
          continue;
        }
        if (i == shopHoveredRow) {
          SDL_SetRenderDrawColor(this->renderer, 60, 60, 90, 140);
          SDL_FRect rowRect = {layout.shopRect.x, layout.shopRect.y + (i * layout.rowHeight),
                               layout.shopRect.w, layout.rowHeight};
          SDL_RenderFillRect(this->renderer, &rowRect);
        }
        const int price = itemPrice(def);
        const std::string line = def->name + " - " + std::to_string(price);
        SDL_Surface* lineSurface =
            TTF_RenderText_Solid(this->font, line.c_str(), line.length(), textColor);
        SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(this->renderer, lineSurface);
        SDL_FRect lineRect = {layout.shopRect.x, layout.shopRect.y + (i * layout.rowHeight),
                              static_cast<float>(lineSurface->w),
                              static_cast<float>(lineSurface->h)};
        SDL_RenderTexture(this->renderer, lineTexture, nullptr, &lineRect);
        SDL_DestroySurface(lineSurface);
        SDL_DestroyTexture(lineTexture);
      }
      if (shopMaxScrollRows > 0) {
        const float trackX = layout.shopRect.x + layout.shopRect.w - 6.0f;
        const float trackY = layout.shopRect.y;
        const float trackH = layout.shopRect.h;
        const float thumbH = std::max(18.0f,
                                      (static_cast<float>(shopVisibleRows) /
                                       static_cast<float>(shop.stock.size())) *
                                          trackH);
        const float scrollRatio =
            (shopMaxScrollRows > 0)
                ? (this->shopScroll /
                   (static_cast<float>(shopMaxScrollRows) * layout.rowHeight))
                : 0.0f;
        const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
        SDL_SetRenderDrawColor(this->renderer, 30, 30, 30, 220);
        SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
        SDL_RenderFillRect(this->renderer, &trackRect);
        SDL_SetRenderDrawColor(this->renderer, 200, 200, 200, 220);
        SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
        SDL_RenderFillRect(this->renderer, &thumbRect);
      }

      for (int i = 0; i < invVisibleRows; ++i) {
        const int index = invStartIndex + i;
        if (index < 0 || index >= static_cast<int>(inventory.items.size())) {
          break;
        }
        const ItemDef* def = this->itemDatabase->getItem(inventory.items[index].itemId);
        if (!def) {
          continue;
        }
        if (i == invHoveredRow) {
          SDL_SetRenderDrawColor(this->renderer, 70, 60, 40, 140);
          SDL_FRect rowRect = {layout.inventoryRect.x,
                               layout.inventoryRect.y + (i * layout.rowHeight),
                               layout.inventoryRect.w, layout.rowHeight};
          SDL_RenderFillRect(this->renderer, &rowRect);
        }
        const int basePrice = itemPrice(def);
        const int sellPrice =
            basePrice > 0 ? std::max(1, basePrice / SHOP_SELL_DIVISOR) : 0;
        const std::string line = def->name + " - " + std::to_string(sellPrice);
        SDL_Surface* lineSurface =
            TTF_RenderText_Solid(this->font, line.c_str(), line.length(), textColor);
        SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(this->renderer, lineSurface);
        SDL_FRect lineRect = {layout.inventoryRect.x,
                              layout.inventoryRect.y + (i * layout.rowHeight),
                              static_cast<float>(lineSurface->w),
                              static_cast<float>(lineSurface->h)};
        SDL_RenderTexture(this->renderer, lineTexture, nullptr, &lineRect);
        SDL_DestroySurface(lineSurface);
        SDL_DestroyTexture(lineTexture);
      }
      if (invMaxScrollRows > 0) {
        const float trackX = layout.inventoryRect.x + layout.inventoryRect.w - 6.0f;
        const float trackY = layout.inventoryRect.y;
        const float trackH = layout.inventoryRect.h;
        const float thumbH =
            std::max(18.0f, (static_cast<float>(invVisibleRows) /
                             static_cast<float>(inventory.items.size())) *
                                trackH);
        const float scrollRatio =
            (invMaxScrollRows > 0)
                ? (this->inventoryScroll /
                   (static_cast<float>(invMaxScrollRows) * layout.rowHeight))
                : 0.0f;
        const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
        SDL_SetRenderDrawColor(this->renderer, 30, 30, 30, 220);
        SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
        SDL_RenderFillRect(this->renderer, &trackRect);
        SDL_SetRenderDrawColor(this->renderer, 200, 200, 200, 220);
        SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
        SDL_RenderFillRect(this->renderer, &thumbRect);
      }

      const std::string hint = "Click item to buy/sell";
      SDL_Surface* hintSurface =
          TTF_RenderText_Solid(this->font, hint.c_str(), hint.length(), hintColor);
      SDL_Texture* hintTexture = SDL_CreateTextureFromSurface(this->renderer, hintSurface);
      SDL_FRect hintRect = {layout.panel.x + 12.0f,
                            layout.panel.y + layout.panel.h - 18.0f,
                            static_cast<float>(hintSurface->w),
                            static_cast<float>(hintSurface->h)};
      SDL_RenderTexture(this->renderer, hintTexture, nullptr, &hintRect);
      SDL_DestroySurface(hintSurface);
      SDL_DestroyTexture(hintTexture);

      if (shopHoveredRow >= 0) {
        const int index = shopStartIndex + shopHoveredRow;
        const ItemDef* def =
            (index >= 0 && index < static_cast<int>(shop.stock.size()))
                ? this->itemDatabase->getItem(shop.stock[index])
                : nullptr;
        if (def) {
          const std::string tip = "Buy for " + std::to_string(itemPrice(def)) + " gold";
          SDL_Surface* tipSurface =
              TTF_RenderText_Solid(this->font, tip.c_str(), tip.length(), headerColor);
          SDL_Texture* tipTexture = SDL_CreateTextureFromSurface(this->renderer, tipSurface);
          SDL_FRect tipRect = {layout.shopRect.x,
                               layout.shopRect.y + (shopHoveredRow * layout.rowHeight) - 18.0f,
                               static_cast<float>(tipSurface->w),
                               static_cast<float>(tipSurface->h)};
          SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
          SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 180);
          SDL_FRect tipBg = {tipRect.x - 4.0f, tipRect.y - 2.0f, tipRect.w + 8.0f,
                             tipRect.h + 4.0f};
          SDL_RenderFillRect(this->renderer, &tipBg);
          SDL_RenderTexture(this->renderer, tipTexture, nullptr, &tipRect);
          SDL_DestroySurface(tipSurface);
          SDL_DestroyTexture(tipTexture);
        }
      }

      if (invHoveredRow >= 0) {
        const int index = invStartIndex + invHoveredRow;
        const ItemDef* def = (index >= 0 && index < static_cast<int>(inventory.items.size()))
                                 ? this->itemDatabase->getItem(inventory.items[index].itemId)
                                 : nullptr;
        if (def) {
          const int basePrice = itemPrice(def);
          const int sellPrice =
              basePrice > 0 ? std::max(1, basePrice / SHOP_SELL_DIVISOR) : 0;
          const std::string tip = "Sell for " + std::to_string(sellPrice) + " gold";
          SDL_Surface* tipSurface =
              TTF_RenderText_Solid(this->font, tip.c_str(), tip.length(), headerColor);
          SDL_Texture* tipTexture = SDL_CreateTextureFromSurface(this->renderer, tipSurface);
          SDL_FRect tipRect = {layout.inventoryRect.x,
                               layout.inventoryRect.y + (invHoveredRow * layout.rowHeight) - 18.0f,
                               static_cast<float>(tipSurface->w),
                               static_cast<float>(tipSurface->h)};
          SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
          SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 180);
          SDL_FRect tipBg = {tipRect.x - 4.0f, tipRect.y - 2.0f, tipRect.w + 8.0f,
                             tipRect.h + 4.0f};
          SDL_RenderFillRect(this->renderer, &tipBg);
          SDL_RenderTexture(this->renderer, tipTexture, nullptr, &tipRect);
          SDL_DestroySurface(tipSurface);
          SDL_DestroyTexture(tipTexture);
        }
      }

      if (this->shopNoticeTimer > 0.0f && !this->shopNotice.empty()) {
        SDL_Surface* noticeSurface =
            TTF_RenderText_Solid(this->font, this->shopNotice.c_str(),
                                 this->shopNotice.length(), headerColor);
        SDL_Texture* noticeTexture = SDL_CreateTextureFromSurface(this->renderer, noticeSurface);
        SDL_FRect noticeRect = {layout.panel.x + 12.0f, layout.panel.y - 20.0f,
                                static_cast<float>(noticeSurface->w),
                                static_cast<float>(noticeSurface->h)};
        SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 180);
        SDL_FRect noticeBg = {noticeRect.x - 6.0f, noticeRect.y - 4.0f, noticeRect.w + 12.0f,
                              noticeRect.h + 8.0f};
        SDL_RenderFillRect(this->renderer, &noticeBg);
        SDL_RenderTexture(this->renderer, noticeTexture, nullptr, &noticeRect);
        SDL_DestroySurface(noticeSurface);
        SDL_DestroyTexture(noticeTexture);
      }
    }
  }

  { // NPC dialog
    if (this->activeNpcId != -1 && !this->shopOpen) {
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const std::string title = npc.name;
      std::string line = npc.dialogLine;
      const QuestLogComponent& questLog =
          this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
      const LevelComponent& level =
          this->registry->getComponent<LevelComponent>(this->playerEntityId);
      const std::vector<QuestEntry> entries = buildNpcQuestEntries(
          npc.name, *this->questSystem, *this->questDatabase, questLog, level);
      if (!entries.empty()) {
        line += "\n\nQuests:";
        for (std::size_t i = 0; i < entries.size(); ++i) {
          const QuestEntry& entry = entries[i];
          const QuestDef* def = entry.def;
          if (!def) {
            continue;
          }
          const std::string prefix =
              (static_cast<int>(i) == this->activeNpcQuestSelection) ? "> " : "  ";
          const std::string type =
              (entry.type == QuestEntryType::Available) ? "[Accept]" : "[Turn In]";
          line += "\n" + prefix + def->name + " " + type;
        }
        line += "\n\nPageUp/PageDown to select";
        line += "\nPress A to accept, C to turn in";
      }
      SDL_Color titleColor = {255, 240, 210, 255};
      SDL_Color textColor = {235, 235, 235, 255};
      constexpr float panelWidth = 360.0f;
      constexpr float panelHeight = 120.0f;
      constexpr float panelPadding = 10.0f;
      constexpr float titleOffsetY = 10.0f;
      constexpr float textOffsetY = 34.0f;
      constexpr float lineHeight = 16.0f;
      const float textAreaHeight = panelHeight - textOffsetY - panelPadding;
      const int maxWidth = static_cast<int>(panelWidth - (panelPadding * 2.0f));
      const std::vector<std::string> lines = wrapText(this->font, line, maxWidth);
      const int totalLines = static_cast<int>(lines.size());
      const int maxVisibleLines =
          std::max(1, static_cast<int>(std::floor(textAreaHeight / lineHeight)));
      const float totalHeight = totalLines * lineHeight;
      const float visibleHeight = maxVisibleLines * lineHeight;
      const float maxScroll = std::max(0.0f, totalHeight - visibleHeight);
      this->npcDialogScroll = std::clamp(this->npcDialogScroll, 0.0f, maxScroll);
      const int startLine = static_cast<int>(std::floor(this->npcDialogScroll / lineHeight));
      const float lineOffset = std::fmod(this->npcDialogScroll, lineHeight);
      const int endLine = std::min(totalLines, startLine + maxVisibleLines + 1);

      SDL_Surface* titleSurface =
          TTF_RenderText_Solid(this->font, title.c_str(), title.length(), titleColor);
      SDL_FRect panel = {12.0f, WINDOW_HEIGHT - panelHeight - 92.0f, panelWidth, panelHeight};
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 10, 10, 10, 210);
      SDL_RenderFillRect(this->renderer, &panel);
      SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 140);
      SDL_RenderRect(this->renderer, &panel);

      SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(this->renderer, titleSurface);
      SDL_FRect titleRect = {panel.x + panelPadding, panel.y + titleOffsetY,
                             static_cast<float>(titleSurface->w),
                             static_cast<float>(titleSurface->h)};
      SDL_RenderTexture(this->renderer, titleTexture, nullptr, &titleRect);

      float textY = panel.y + textOffsetY - lineOffset;
      for (int i = startLine; i < endLine; ++i) {
        SDL_Surface* lineSurface =
            TTF_RenderText_Solid(this->font, lines[i].c_str(), lines[i].length(), textColor);
        SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(this->renderer, lineSurface);
        SDL_FRect lineRect = {panel.x + panelPadding, textY,
                              static_cast<float>(lineSurface->w),
                              static_cast<float>(lineSurface->h)};
        if (lineRect.y + lineRect.h >= panel.y + textOffsetY - 2.0f &&
            lineRect.y <= panel.y + panelHeight - panelPadding) {
          SDL_RenderTexture(this->renderer, lineTexture, nullptr, &lineRect);
        }
        SDL_DestroyTexture(lineTexture);
        SDL_DestroySurface(lineSurface);
        textY += lineHeight;
      }

      if (maxScroll > 0.0f) {
        const float trackX = panel.x + panel.w - 8.0f;
        const float trackY = panel.y + textOffsetY;
        const float trackH = visibleHeight;
        const float thumbH = std::max(18.0f, (visibleHeight / totalHeight) * trackH);
        const float scrollRatio = (maxScroll > 0.0f) ? (this->npcDialogScroll / maxScroll) : 0.0f;
        const float thumbY = trackY + (trackH - thumbH) * scrollRatio;
        SDL_SetRenderDrawColor(this->renderer, 30, 30, 30, 220);
        SDL_FRect trackRect = {trackX, trackY, 4.0f, trackH};
        SDL_RenderFillRect(this->renderer, &trackRect);
        SDL_SetRenderDrawColor(this->renderer, 200, 200, 200, 220);
        SDL_FRect thumbRect = {trackX, thumbY, 4.0f, thumbH};
        SDL_RenderFillRect(this->renderer, &thumbRect);
      }

      SDL_DestroyTexture(titleTexture);
      SDL_DestroySurface(titleSurface);
    }
  }

  if (!this->isPlayerGhost) {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
    const float cooldown = attackProfile.cooldown;
    const float ratio =
        (cooldown > 0.0f)
            ? std::clamp(1.0f - (this->attackCooldownRemaining / cooldown), 0.0f, 1.0f)
            : 1.0f;
    SDL_FRect barBg = {playerTransform.position.x - cameraPosition.x,
                       playerTransform.position.y - cameraPosition.y + playerCollision.height + 4.0f,
                       playerCollision.width, 4.0f};
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(this->renderer, &barBg);
    SDL_FRect barFill = barBg;
    barFill.w = barBg.w * ratio;
    if (this->attackCooldownRemaining > 0.0f) {
      SDL_SetRenderDrawColor(this->renderer, 230, 180, 60, 220);
    } else {
      SDL_SetRenderDrawColor(this->renderer, 100, 220, 120, 220);
    }
    SDL_RenderFillRect(this->renderer, &barFill);
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 80);
    SDL_RenderRect(this->renderer, &barBg);
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

  if (this->isPlayerGhost && this->hasCorpse) {
    const TransformComponent& playerTransform =
        this->registry->getComponent<TransformComponent>(this->playerEntityId);
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const Position playerCenter = centerForEntity(playerTransform, playerCollision);
    const Position corpseCenter(this->corpsePosition.x + (playerCollision.width / 2.0f),
                                this->corpsePosition.y + (playerCollision.height / 2.0f));
    const float distToCorpse = squaredDistance(playerCenter, corpseCenter);
    const bool inRange = distToCorpse <= (RESURRECT_RANGE * RESURRECT_RANGE);
    const std::string prompt =
        inRange ? "Press R to resurrect" : "You are a spirit. Return to your corpse.";
    SDL_Color textColor = {255, 240, 200, 255};
    SDL_Surface* surface =
        TTF_RenderText_Solid(this->font, prompt.c_str(), prompt.length(), textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    SDL_FRect textRect = {12.0f, 40.0f, static_cast<float>(surface->w),
                          static_cast<float>(surface->h)};
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 160);
    SDL_FRect bgRect = {textRect.x - 6.0f, textRect.y - 4.0f, textRect.w + 12.0f,
                        textRect.h + 8.0f};
    SDL_RenderFillRect(this->renderer, &bgRect);
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
    const QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    std::vector<MinimapMarker> markers;
    for (const QuestProgress& progress : questLog.activeQuests) {
      const QuestDef* def = this->questDatabase->getQuest(progress.questId);
      if (!def) {
        continue;
      }
      if (!def->turnInNpcName.empty() && progress.completed && !progress.rewardsClaimed) {
        for (int npcId : this->npcEntityIds) {
          const NpcComponent& npc = this->registry->getComponent<NpcComponent>(npcId);
          if (npc.name != def->turnInNpcName) {
            continue;
          }
          const TransformComponent& npcTransform =
              this->registry->getComponent<TransformComponent>(npcId);
          const float tileX = npcTransform.position.x / TILE_SIZE;
          const float tileY = npcTransform.position.y / TILE_SIZE;
          markers.push_back(MinimapMarker{tileX, tileY, SDL_Color{255, 220, 80, 255}});
        }
      }
      for (const QuestObjectiveProgress& objective : progress.objectives) {
        if (objective.def.type != QuestObjectiveType::EnterRegion) {
          continue;
        }
        if (objective.currentCount >= objective.def.requiredCount) {
          continue;
        }
        const std::optional<Position> regionCenter =
            regionCenterByName(*this->map, objective.def.regionName);
        if (regionCenter.has_value()) {
          markers.push_back(MinimapMarker{regionCenter->x / TILE_SIZE,
                                          regionCenter->y / TILE_SIZE,
                                          SDL_Color{120, 220, 255, 255}});
        }
      }
    }
    this->minimap->render(this->renderer, *this->map, playerTransform.position, WINDOW_WIDTH,
                          WINDOW_HEIGHT, markers);
  }

  { // Character stats overlay
    const HealthComponent& health =
        this->registry->getComponent<HealthComponent>(this->playerEntityId);
    const ManaComponent& mana = this->registry->getComponent<ManaComponent>(this->playerEntityId);
    const LevelComponent& level =
        this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const StatsComponent& stats =
        this->registry->getComponent<StatsComponent>(this->playerEntityId);
    const ClassComponent& playerClass =
        this->registry->getComponent<ClassComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const int attackPower = computeAttackPower(stats, equipment, *this->itemDatabase);
    this->characterStats->render(this->renderer, this->font, WINDOW_WIDTH, health, mana, level,
                                 attackPower, className(playerClass.characterClass),
                                 stats.strength, stats.gold, stats.dexterity, stats.intellect,
                                 stats.luck,
                                 stats.unspentPoints,
                                 this->inventoryUi->isStatsVisible());
  }

  { // Quest log overlay
    const QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    this->questLogUi->render(this->renderer, this->font, questLog, *this->questDatabase,
                             *this->itemDatabase, WINDOW_WIDTH, WINDOW_HEIGHT,
                             this->questLogVisible, this->questLogScroll);
  }

  {
    const SkillBarComponent& skills =
        this->registry->getComponent<SkillBarComponent>(this->playerEntityId);
    const SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    this->skillBar->render(this->renderer, this->font, skills, skillTree, *this->skillDatabase,
                           WINDOW_WIDTH, WINDOW_HEIGHT);
  }

  {
    const BuffComponent& buffs = this->registry->getComponent<BuffComponent>(this->playerEntityId);
    this->buffBar->render(this->renderer, this->font, buffs);
  }

  {
    const SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    this->skillTree->render(this->renderer, this->font, skillTree, *this->skillTreeDefinition,
                            *this->skillDatabase, WINDOW_WIDTH, WINDOW_HEIGHT);
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
