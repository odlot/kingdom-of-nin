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
#include "ecs/component/pushback_component.h"
#include "ecs/component/quest_log_component.h"
#include "ecs/component/shop_component.h"
#include "ecs/component/skill_bar_component.h"
#include "ecs/component/skill_tree_component.h"
#include "ecs/component/stats_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/system/graphic_system.h"
#include "ecs/system/movement_system.h"
#include "ecs/system/pushback_system.h"
#include "events/events.h"
#include "gameplay/projectile_system.h"
#include "quests/quest_helpers.h"
#include "ui/buff_bar.h"
#include "ui/floating_text_system.h"
#include "ui/inventory.h"
#include "ui/minimap.h"
#include "ui/npc_dialog.h"
#include "ui/quest_log.h"
#include "ui/quest_marker_helpers.h"
#include "ui/render_utils.h"
#include "ui/skill_bar.h"
#include "ui/skill_tree.h"
#include "world/generator.h"
#include "world/region.h"
#include "world/tile.h"
#include <SDL3/SDL_keyboard.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
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
constexpr int PLAYER_LEVEL_CAP = 60;
constexpr float FACING_TURN_SPEED = 8.0f;
constexpr float PUSHBACK_DISTANCE = static_cast<float>(TILE_SIZE);
constexpr float PUSHBACK_DURATION = 0.2f;
constexpr float PLAYER_KNOCKBACK_IMMUNITY_SECONDS = 2.0f;
constexpr float RESURRECT_RANGE = 28.0f;

namespace {
constexpr unsigned int kCombatSeedSalt = 0xA53F91U;
constexpr unsigned int kLootSeedSalt = 0xBADC0DEU;
constexpr unsigned int kSpawnSeedSalt = 0x51EED123U;

unsigned int readWorldSeed() {
  const char* seedText = std::getenv("KINGDOM_OF_NIN_SEED");
  if (!seedText || seedText[0] == '\0') {
    return std::random_device{}();
  }
  char* end = nullptr;
  const unsigned long parsed = std::strtoul(seedText, &end, 10);
  if (!end || *end != '\0') {
    return std::random_device{}();
  }
  return static_cast<unsigned int>(parsed);
}

unsigned int deriveSeed(unsigned int baseSeed, unsigned int salt) {
  return baseSeed ^ (salt + 0x9E3779B9U + (baseSeed << 6U) + (baseSeed >> 2U));
}

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

constexpr std::array<CharacterClass, 4> kClassUnlockChoices = {
    CharacterClass::Warrior, CharacterClass::Mage, CharacterClass::Archer, CharacterClass::Rogue};

const char* classUnlockLabel(CharacterClass characterClass) {
  switch (characterClass) {
  case CharacterClass::Warrior:
    return "F1 - Warrior";
  case CharacterClass::Mage:
    return "F2 - Mage";
  case CharacterClass::Archer:
    return "F3 - Archer";
  case CharacterClass::Rogue:
    return "F4 - Rogue";
  case CharacterClass::Any:
    break;
  }
  return "";
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
  switch (def->rarity) {
  case ItemRarity::Common:
    return SDL_Color{210, 210, 210, 255};
  case ItemRarity::Rare:
    return SDL_Color{100, 160, 255, 255};
  case ItemRarity::Epic:
    return SDL_Color{200, 120, 255, 255};
  }
  return SDL_Color{200, 200, 200, 255};
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

bool isShopNpc(int npcId, const std::vector<int>& shopNpcIds) {
  return std::find(shopNpcIds.begin(), shopNpcIds.end(), npcId) != shopNpcIds.end();
}

bool pointInRect(float x, float y, const SDL_FRect& rect) {
  return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

} // namespace

Game::InputState Game::captureInput() {
  InputState input;
  input.keyboardState = SDL_GetKeyboardState(nullptr);

  int x = 0;
  int y = 0;
  if (input.keyboardState[SDL_SCANCODE_LEFT]) {
    x = -1;
  }
  if (input.keyboardState[SDL_SCANCODE_DOWN]) {
    y = 1;
  }
  if (input.keyboardState[SDL_SCANCODE_UP]) {
    y = -1;
  }
  if (input.keyboardState[SDL_SCANCODE_RIGHT]) {
    x = 1;
  }
  input.moveX = x;
  input.moveY = y;

  input.pickupPressed = input.keyboardState[SDL_SCANCODE_F];
  input.interactPressed = input.keyboardState[SDL_SCANCODE_T];
  input.debugPressed = input.keyboardState[SDL_SCANCODE_D];
  input.resurrectPressed = input.keyboardState[SDL_SCANCODE_R];
  input.questLogPressed = input.keyboardState[SDL_SCANCODE_Q];
  input.acceptQuestPressed = input.keyboardState[SDL_SCANCODE_A];
  input.turnInQuestPressed = input.keyboardState[SDL_SCANCODE_C];
  input.questPrevPressed = input.keyboardState[SDL_SCANCODE_PAGEUP];
  input.questNextPressed = input.keyboardState[SDL_SCANCODE_PAGEDOWN];
  input.classMenuPressed = input.keyboardState[SDL_SCANCODE_K];

  const std::array<SDL_Scancode, 5> skillKeys = {SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
                                                 SDL_SCANCODE_4, SDL_SCANCODE_5};
  for (std::size_t i = 0; i < skillKeys.size(); ++i) {
    input.skillPressed[i] = input.keyboardState[skillKeys[i]];
    input.skillJustPressed[i] = input.skillPressed[i] && !this->wasSkillPressed[i];
    this->wasSkillPressed[i] = input.skillPressed[i];
  }

  const std::array<SDL_Scancode, 4> classChoiceKeys = {SDL_SCANCODE_F1, SDL_SCANCODE_F2,
                                                       SDL_SCANCODE_F3, SDL_SCANCODE_F4};
  for (std::size_t i = 0; i < classChoiceKeys.size(); ++i) {
    input.classChoicePressed[i] = input.keyboardState[classChoiceKeys[i]];
    input.classChoiceJustPressed[i] =
        input.classChoicePressed[i] && !this->wasClassChoicePressed[i];
    this->wasClassChoicePressed[i] = input.classChoicePressed[i];
  }

  float mouseX = 0.0f;
  float mouseY = 0.0f;
  const Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
  input.mouseX = mouseX;
  input.mouseY = mouseY;
  input.mousePressed = (mouseState & SDL_BUTTON_LMASK) != 0;
  input.click = input.mousePressed && !this->wasMousePressed;
  this->wasMousePressed = input.mousePressed;

  input.pickupJustPressed = input.pickupPressed && !this->wasPickupPressed;
  this->wasPickupPressed = input.pickupPressed;
  input.interactJustPressed = input.interactPressed && !this->wasInteractPressed;
  this->wasInteractPressed = input.interactPressed;
  input.debugJustPressed = input.debugPressed && !this->wasDebugPressed;
  this->wasDebugPressed = input.debugPressed;
  input.resurrectJustPressed = input.resurrectPressed && !this->wasResurrectPressed;
  this->wasResurrectPressed = input.resurrectPressed;
  input.questLogJustPressed = input.questLogPressed && !this->wasQuestLogPressed;
  this->wasQuestLogPressed = input.questLogPressed;
  input.acceptQuestJustPressed = input.acceptQuestPressed && !this->wasAcceptQuestPressed;
  this->wasAcceptQuestPressed = input.acceptQuestPressed;
  input.turnInQuestJustPressed = input.turnInQuestPressed && !this->wasTurnInQuestPressed;
  this->wasTurnInQuestPressed = input.turnInQuestPressed;
  input.questPrevJustPressed = input.questPrevPressed && !this->wasQuestPrevPressed;
  this->wasQuestPrevPressed = input.questPrevPressed;
  input.questNextJustPressed = input.questNextPressed && !this->wasQuestNextPressed;
  this->wasQuestNextPressed = input.questNextPressed;
  input.classMenuJustPressed = input.classMenuPressed && !this->wasClassMenuPressed;
  this->wasClassMenuPressed = input.classMenuPressed;

  input.mouseWheelDelta = this->mouseWheelDelta;
  this->mouseWheelDelta = 0.0f;

  return input;
}

int computeAttackPower(const StatsComponent& stats, const EquipmentComponent& equipment,
                       const ItemDatabase& database, CharacterClass characterClass) {
  int strength = stats.strength;
  int dexterity = stats.dexterity;
  int intellect = stats.intellect;
  int luck = stats.luck;
  for (const auto& entry : equipment.equipped) {
    const ItemDef* def = database.getItem(entry.second.itemId);
    if (!def) {
      continue;
    }
    const PrimaryStatBonuses& primary = primaryStatsForItem(*def);
    strength += primary.strength;
    dexterity += primary.dexterity;
    intellect += primary.intellect;
    luck += primary.luck;
  }
  int primaryStat = strength;
  switch (characterClass) {
  case CharacterClass::Warrior:
    primaryStat = strength;
    break;
  case CharacterClass::Archer:
    primaryStat = dexterity;
    break;
  case CharacterClass::Mage:
    primaryStat = intellect;
    break;
  case CharacterClass::Rogue:
    primaryStat = luck;
    break;
  case CharacterClass::Any:
    primaryStat = std::max({strength, dexterity, intellect, luck});
    break;
  }
  return stats.baseAttackPower + primaryStat;
}

int computeArmor(const StatsComponent& stats, const EquipmentComponent& equipment,
                 const ItemDatabase& database) {
  int total = stats.baseArmor;
  for (const auto& entry : equipment.equipped) {
    const ItemDef* def = database.getItem(entry.second.itemId);
    if (!def) {
      continue;
    }
    total += armorForItem(*def);
  }
  return total;
}

struct EffectivePrimaryStats {
  int strength = 0;
  int dexterity = 0;
  int intellect = 0;
  int luck = 0;
};

EffectivePrimaryStats computeEffectivePrimaryStats(const StatsComponent& stats,
                                                   const EquipmentComponent& equipment,
                                                   const ItemDatabase& database) {
  EffectivePrimaryStats effective{stats.strength, stats.dexterity, stats.intellect, stats.luck};
  for (const auto& entry : equipment.equipped) {
    const ItemDef* def = database.getItem(entry.second.itemId);
    if (!def) {
      continue;
    }
    const PrimaryStatBonuses& primary = primaryStatsForItem(*def);
    effective.strength += primary.strength;
    effective.dexterity += primary.dexterity;
    effective.intellect += primary.intellect;
    effective.luck += primary.luck;
  }
  return effective;
}

void refreshSecondaryStats(StatsComponent& stats, const EffectivePrimaryStats& effectiveStats) {
  stats.critChanceBonus =
      std::clamp(0.0025f * static_cast<float>(effectiveStats.luck), 0.0f, 0.35f);
  stats.critDamageBonus = std::clamp(0.01f * static_cast<float>(effectiveStats.luck), 0.0f, 1.2f);
  stats.accuracy = std::clamp(0.72f + (0.012f * static_cast<float>(effectiveStats.dexterity)) +
                                  (0.0015f * static_cast<float>(effectiveStats.luck)),
                              0.72f, 0.99f);
  stats.dodge = std::clamp((0.0025f * static_cast<float>(effectiveStats.dexterity)) +
                               (0.001f * static_cast<float>(effectiveStats.luck)),
                           0.0f, 0.35f);
  stats.parry = std::clamp((0.0015f * static_cast<float>(effectiveStats.strength)) +
                               (0.001f * static_cast<float>(effectiveStats.dexterity)),
                           0.0f, 0.25f);
}

float playerCritChance(const StatsComponent& stats) {
  return std::clamp(0.05f + stats.critChanceBonus, 0.05f, 0.65f);
}

float playerCritMultiplier(const StatsComponent& stats) {
  return std::clamp(1.5f + stats.critDamageBonus, 1.25f, 3.0f);
}

float mobEvasionChance(const MobComponent& mob) {
  float base = 0.02f;
  switch (mob.behavior) {
  case MobBehaviorType::Melee:
    base = 0.03f;
    break;
  case MobBehaviorType::Ranged:
    base = 0.055f;
    break;
  case MobBehaviorType::Caster:
    base = 0.045f;
    break;
  case MobBehaviorType::Bruiser:
    base = 0.015f;
    break;
  case MobBehaviorType::Skirmisher:
    base = 0.06f;
    break;
  }
  return std::clamp(base + (0.0018f * static_cast<float>(mob.level)), 0.01f, 0.25f);
}

float squaredDistance(const Position& a, const Position& b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return (dx * dx) + (dy * dy);
}

Position retreatTarget(const Position& mobCenter, const Position& playerCenter, float currentX,
                       float currentY) {
  float dx = mobCenter.x - playerCenter.x;
  float dy = mobCenter.y - playerCenter.y;
  const float length = std::sqrt((dx * dx) + (dy * dy));
  if (length <= 0.001f) {
    return Position(currentX, currentY);
  }
  dx /= length;
  dy /= length;
  return Position(currentX + (dx * TILE_SIZE * 2.0f), currentY + (dy * TILE_SIZE * 2.0f));
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
  while (level.level < PLAYER_LEVEL_CAP && level.experience >= level.nextLevelExperience) {
    level.experience -= level.nextLevelExperience;
    level.level += 1;
    level.nextLevelExperience += 100;
    stats.unspentPoints += 5;
    skillTree.unspentPoints += 1;
  }
  if (level.level >= PLAYER_LEVEL_CAP) {
    level.level = PLAYER_LEVEL_CAP;
    level.experience = 0;
    level.nextLevelExperience = 0;
  }
}

bool equipItemByIndex(InventoryComponent& inventory, EquipmentComponent& equipment,
                      const ItemDatabase& database, std::size_t index, int playerLevel,
                      CharacterClass playerClass) {
  std::optional<ItemInstance> item = inventory.takeItemAt(index);
  if (!item.has_value()) {
    return false;
  }
  const ItemDef* def = database.getItem(item->itemId);
  if (!def) {
    inventory.addItem(*item);
    return false;
  }
  if (!meetsEquipRequirements(*def, playerLevel, playerClass)) {
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
                   float distance, float duration);
bool createLootEntity(Registry& registry, const ItemDatabase& database, const Position& position,
                      int itemId, std::vector<int>& lootEntityIds);
int createProjectileEntity(Registry& registry, const Position& position, float velocityX,
                           float velocityY, float range, int sourceEntityId, int targetEntityId,
                           int damage, bool isCrit, float radius, float trailLength,
                           SDL_Color color, std::vector<int>& projectileEntityIds);
void moveEntityToward(const Map& map, TransformComponent& transform,
                      const CollisionComponent& collision, float speed, const Position& target,
                      float dt);

void Game::updateNpcInteraction(const InputState& input) {
  const Position playerCenter = this->playerCenter();
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
  if (input.interactJustPressed) {
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
      StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
      LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
      InventoryComponent& inventory =
          this->registry->getComponent<InventoryComponent>(this->playerEntityId);
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const std::vector<std::string> turnedIn =
          this->questSystem->tryTurnIn(npc.name, questLog, stats, level, inventory);
      if (!turnedIn.empty()) {
        const Position playerCenter = this->playerCenter();
        for (const std::string& questName : turnedIn) {
          this->eventBus->emitFloatingTextEvent(
              FloatingTextEvent{"Completed: " + questName, playerCenter, FloatingTextKind::Info});
        }
      }
    }
  }
  if (this->activeNpcId != -1 && !this->shopOpen && input.acceptQuestJustPressed) {
    QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
    const std::vector<QuestEntry> entries =
        buildNpcQuestEntries(npc.name, *this->questSystem, *this->questDatabase, questLog, level);
    if (!entries.empty() && this->activeNpcQuestSelection >= 0 &&
        this->activeNpcQuestSelection < static_cast<int>(entries.size()) &&
        entries[this->activeNpcQuestSelection].type == QuestEntryType::Available) {
      const QuestDef* def = entries[this->activeNpcQuestSelection].def;
      if (def) {
        this->questSystem->addQuest(questLog, level, def->id);
        const Position playerCenter = this->playerCenter();
        this->eventBus->emitFloatingTextEvent(
            FloatingTextEvent{"Accepted: " + def->name, playerCenter, FloatingTextKind::Info});
      }
    }
  }
  if (this->activeNpcId != -1 && !this->shopOpen && input.turnInQuestJustPressed) {
    QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
    const std::vector<QuestEntry> entries =
        buildNpcQuestEntries(npc.name, *this->questSystem, *this->questDatabase, questLog, level);
    if (!entries.empty() && this->activeNpcQuestSelection >= 0 &&
        this->activeNpcQuestSelection < static_cast<int>(entries.size()) &&
        entries[this->activeNpcQuestSelection].type == QuestEntryType::TurnIn) {
      const QuestDef* def = entries[this->activeNpcQuestSelection].def;
      if (def) {
        const std::vector<std::string> turnedIn =
            this->questSystem->tryTurnIn(npc.name, questLog, stats, level, inventory);
        if (!turnedIn.empty()) {
          const Position playerCenter = this->playerCenter();
          for (const std::string& questName : turnedIn) {
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{"Completed: " + questName, playerCenter, FloatingTextKind::Info});
          }
        }
      }
    }
  }
  if (this->activeNpcId != -1 && !this->shopOpen &&
      (input.questPrevJustPressed || input.questNextJustPressed)) {
    QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
    const std::vector<QuestEntry> entries =
        buildNpcQuestEntries(npc.name, *this->questSystem, *this->questDatabase, questLog, level);
    if (!entries.empty()) {
      const int count = static_cast<int>(entries.size());
      if (input.questNextJustPressed) {
        this->activeNpcQuestSelection = (this->activeNpcQuestSelection + 1) % count;
      } else if (input.questPrevJustPressed) {
        this->activeNpcQuestSelection = (this->activeNpcQuestSelection - 1 + count) % count;
      }
    } else {
      this->activeNpcQuestSelection = 0;
    }
  }
  if (this->activeNpcId != -1 && !this->shopOpen && input.mouseWheelDelta != 0.0f) {
    this->npcDialogScroll -= input.mouseWheelDelta * 18.0f;
  }
}

void Game::updateAutoTargetAndFacing(const InputState& input, float dt) {
  if (this->isPlayerGhost) {
    this->currentAutoTargetId = -1;
    return;
  }

  const EquipmentComponent& equipment =
      this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
  const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
  const Position playerCenter = this->playerCenter();
  const float range = attackProfile.range * AUTO_TARGET_RANGE_MULTIPLIER;
  const float rangeSquared = range * range;
  int closestMobId = -1;
  float closestDist = rangeSquared;
  Position closestCenter;
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
    desiredAngle = std::atan2(closestCenter.y - playerCenter.y, closestCenter.x - playerCenter.x);
    hasFacingTarget = true;
  } else if (input.moveX != 0 || input.moveY != 0) {
    desiredAngle = std::atan2(static_cast<float>(input.moveY), static_cast<float>(input.moveX));
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

void Game::updatePlayerAttack(float dt) {
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
      const ClassComponent& playerClass =
          this->registry->getComponent<ClassComponent>(this->playerEntityId);
      const MobComponent& mob = this->registry->getComponent<MobComponent>(mobEntityId);
      this->eventBus->emitMobKilledEvent(MobKilledEvent{mob.type, mobEntityId});
      level.experience += mob.experience;
      this->eventBus->emitFloatingTextEvent(FloatingTextEvent{
          "XP +" + std::to_string(mob.experience), hitPosition, FloatingTextKind::Info});
      EquipmentDropGenerationOptions dropOptions;
      if (this->mobDatabase->rollEquipmentDrop(mob.type, this->lootRng, dropOptions)) {
        const int dropLevel = std::clamp(level.level, 1, PLAYER_LEVEL_CAP);
        const int droppedItemId = this->itemDatabase->generateEquipmentDrop(
            dropLevel, playerClass.characterClass, this->lootRng, dropOptions);
        const TransformComponent& mobTransform =
            this->registry->getComponent<TransformComponent>(mobEntityId);
        createLootEntity(*this->registry, *this->itemDatabase, mobTransform.position, droppedItemId,
                         this->lootEntityIds);
      }
      GraphicComponent& mobGraphic = this->registry->getComponent<GraphicComponent>(mobEntityId);
      mobGraphic.color = SDL_Color({80, 80, 80, 255});
    }
  };

  updateProjectiles(dt, *this->registry, *this->map, *this->respawnSystem,
                    this->projectileEntityIds, this->playerEntityId,
                    [&](int mobId, int damage, bool isCrit, const Position& hitPosition,
                        const Position& fromPosition) {
                      applyPlayerDamageToMob(mobId, damage, isCrit, hitPosition, fromPosition);
                    });

  if (this->isPlayerGhost || this->attackCooldownRemaining > 0.0f) {
    return;
  }
  if (this->currentAutoTargetId == -1) {
    return;
  }

  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  const StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
  const EquipmentComponent& equipment =
      this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
  const ClassComponent& playerClass =
      this->registry->getComponent<ClassComponent>(this->playerEntityId);
  const int attackPower =
      computeAttackPower(stats, equipment, *this->itemDatabase, playerClass.characterClass);
  const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
  std::uniform_real_distribution<float> chanceRoll(0.0f, 1.0f);

  const TransformComponent& mobTransform =
      this->registry->getComponent<TransformComponent>(this->currentAutoTargetId);
  const MobComponent& mob = this->registry->getComponent<MobComponent>(this->currentAutoTargetId);
  const Position mobCenter(mobTransform.position.x + (TILE_SIZE / 2.0f),
                           mobTransform.position.y + (TILE_SIZE / 2.0f));
  const Position playerCenter = this->playerCenter();
  const float rangeSquared = attackProfile.range * attackProfile.range;
  if (squaredDistance(playerCenter, mobCenter) > rangeSquared) {
    return;
  }
  const float hitChance = std::clamp(stats.accuracy - mobEvasionChance(mob), 0.2f, 0.99f);
  if (chanceRoll(this->rng) > hitChance) {
    this->eventBus->emitFloatingTextEvent(
        FloatingTextEvent{"Miss", mobCenter, FloatingTextKind::Info});
    this->attackCooldownRemaining = attackProfile.cooldown;
    return;
  }
  const bool isCrit = chanceRoll(this->rng) <= playerCritChance(stats);
  const int attackDamage = isCrit ? static_cast<int>(std::round(static_cast<float>(attackPower) *
                                                                playerCritMultiplier(stats)))
                                  : attackPower;

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
      createProjectileEntity(*this->registry, spawnPosition, dx * attackProfile.projectileSpeed,
                             dy * attackProfile.projectileSpeed, attackProfile.range,
                             this->playerEntityId, this->currentAutoTargetId, attackDamage, isCrit,
                             attackProfile.projectileRadius, attackProfile.projectileTrailLength,
                             attackProfile.projectileColor, this->projectileEntityIds);
      this->attackCooldownRemaining = attackProfile.cooldown;
    }
  } else {
    applyPlayerDamageToMob(this->currentAutoTargetId, attackDamage, isCrit, mobCenter,
                           playerCenter);
    this->attackCooldownRemaining = attackProfile.cooldown;
  }
}

void Game::updateMobBehavior(float dt) {
  const TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const Position playerCenter = this->playerCenter();
  const int playerTileX = static_cast<int>(playerCenter.x / TILE_SIZE);
  const int playerTileY = static_cast<int>(playerCenter.y / TILE_SIZE);

  for (int mobEntityId : this->mobEntityIds) {
    HealthComponent& mobHealth = this->registry->getComponent<HealthComponent>(mobEntityId);
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

    const bool pursuingPlayer =
        playerAlive && playerInRegion && distToPlayer <= (mob.aggroRange * mob.aggroRange);
    std::optional<Position> target;
    if (pursuingPlayer) {
      const float preferredRange = std::max(16.0f, mob.preferredRange);
      const float preferredRangeSquared = preferredRange * preferredRange;
      const float retreatRangeSquared = (preferredRange * 0.65f) * (preferredRange * 0.65f);
      switch (mob.behavior) {
      case MobBehaviorType::Ranged:
      case MobBehaviorType::Caster:
      case MobBehaviorType::Skirmisher:
        if (distToPlayer < retreatRangeSquared) {
          target = retreatTarget(mobCenter, playerCenter, mobTransform.position.x,
                                 mobTransform.position.y);
        } else if (distToPlayer > preferredRangeSquared) {
          target = playerTransform.position;
        }
        break;
      case MobBehaviorType::Melee:
      case MobBehaviorType::Bruiser:
        target = playerTransform.position;
        break;
      }
    } else if (distToHome > 4.0f) {
      target = homePosition;
    }

    if (target.has_value()) {
      float movementSpeed = mob.speed;
      if (pursuingPlayer && mob.behavior == MobBehaviorType::Bruiser &&
          distToPlayer > (mob.attackRange * mob.attackRange)) {
        movementSpeed *= 1.08f;
      }
      moveEntityToward(*this->map, mobTransform, mobCollision, movementSpeed, *target, dt);
    }

    mob.attackTimer = std::max(0.0f, mob.attackTimer - dt);
    mob.abilityTimer = std::max(0.0f, mob.abilityTimer - dt);
    if (mob.attackTimer <= 0.0f) {
      const float attackRangeSquared = mob.attackRange * mob.attackRange;
      if (playerAlive && squaredDistance(mobCenter, playerCenter) <= attackRangeSquared) {
        HealthComponent& playerHealth =
            this->registry->getComponent<HealthComponent>(this->playerEntityId);
        const LevelComponent& playerLevel =
            this->registry->getComponent<LevelComponent>(this->playerEntityId);
        const StatsComponent& playerStats =
            this->registry->getComponent<StatsComponent>(this->playerEntityId);
        const EquipmentComponent& playerEquipment =
            this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
        if (playerHealth.current > 0) {
          std::uniform_real_distribution<float> chanceRoll(0.0f, 1.0f);
          const float levelPenalty =
              std::max(0.0f, static_cast<float>(mob.level - playerLevel.level) * 0.004f);
          const float parryChance = std::clamp(playerStats.parry - levelPenalty, 0.0f, 0.30f);
          const float dodgeChance =
              std::clamp(playerStats.dodge - (levelPenalty * 1.25f), 0.0f, 0.45f);
          const float avoidRoll = chanceRoll(this->rng);
          if (avoidRoll <= parryChance) {
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{"Parry", playerCenter, FloatingTextKind::Info});
            mob.attackTimer = mob.attackCooldown;
            continue;
          }
          if (avoidRoll <= parryChance + dodgeChance) {
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{"Dodge", playerCenter, FloatingTextKind::Info});
            mob.attackTimer = mob.attackCooldown;
            continue;
          }

          int rawDamage = mob.attackDamage;
          float mitigationMultiplier = 1.0f;
          float knockbackMultiplier = 1.0f;
          int healOnHit = 0;
          const char* abilityLabel = nullptr;
          if (mob.abilityTimer <= 0.0f) {
            switch (mob.abilityType) {
            case MobAbilityType::GoblinRage:
              if ((mobHealth.current * 10) <= (mobHealth.max * 6)) {
                rawDamage = std::max(
                    1, static_cast<int>(std::round(rawDamage * (1.0f + mob.abilityValue))));
                abilityLabel = "Rage";
                mob.abilityTimer = mob.abilityCooldown;
              }
              break;
            case MobAbilityType::UndeadDrain:
              healOnHit = std::max(
                  1, static_cast<int>(std::round(rawDamage * std::max(0.12f, mob.abilityValue))));
              abilityLabel = "Drain";
              mob.abilityTimer = mob.abilityCooldown;
              break;
            case MobAbilityType::BeastPounce:
              if (distToPlayer > (mob.attackRange * mob.attackRange * 1.2f)) {
                rawDamage = std::max(
                    1, static_cast<int>(std::round(rawDamage * (1.0f + mob.abilityValue))));
                knockbackMultiplier = 1.5f;
                abilityLabel = "Pounce";
                mob.abilityTimer = mob.abilityCooldown;
              }
              break;
            case MobAbilityType::BanditTrick:
              if (chanceRoll(this->rng) <= mob.abilityValue) {
                rawDamage = std::max(
                    1, static_cast<int>(std::round(rawDamage * (1.0f + mob.abilityValue))));
                abilityLabel = "Trick";
                mob.abilityTimer = mob.abilityCooldown;
              }
              break;
            case MobAbilityType::ArcaneSurge:
              mitigationMultiplier = std::clamp(1.0f - mob.abilityValue, 0.35f, 1.0f);
              rawDamage += std::max(1, mob.level / 5);
              abilityLabel = "Surge";
              mob.abilityTimer = mob.abilityCooldown;
              break;
            case MobAbilityType::None:
              break;
            }
          }

          const int armor = computeArmor(playerStats, playerEquipment, *this->itemDatabase);
          const float mitigation =
              std::clamp(static_cast<float>(armor) / (100.0f + static_cast<float>(armor)), 0.0f,
                         0.60f) *
              mitigationMultiplier;
          const int damageTaken =
              std::max(1, static_cast<int>(std::round(rawDamage * (1.0f - mitigation))));
          playerHealth.current = std::max(0, playerHealth.current - damageTaken);
          this->eventBus->emitDamageEvent(
              DamageEvent{mobEntityId, this->playerEntityId, damageTaken, playerCenter});
          this->eventBus->emitFloatingTextEvent(FloatingTextEvent{
              "-" + std::to_string(damageTaken), playerCenter, FloatingTextKind::Damage});
          if (abilityLabel) {
            this->eventBus->emitFloatingTextEvent(
                FloatingTextEvent{abilityLabel, mobCenter, FloatingTextKind::Info});
          }
          if (healOnHit > 0) {
            const int healed = std::min(healOnHit, mobHealth.max - mobHealth.current);
            if (healed > 0) {
              mobHealth.current += healed;
              this->eventBus->emitFloatingTextEvent(FloatingTextEvent{
                  "+" + std::to_string(healed), mobCenter, FloatingTextKind::Heal});
            }
          }
          if (this->playerKnockbackImmunityRemaining <= 0.0f) {
            applyPushback(*this->registry, this->playerEntityId, mobCenter,
                          PUSHBACK_DISTANCE * knockbackMultiplier, PUSHBACK_DURATION);
            this->playerKnockbackImmunityRemaining = PLAYER_KNOCKBACK_IMMUNITY_SECONDS;
          }
          this->playerHitFlashTimer = 0.2f;
          mob.attackTimer = mob.attackCooldown;
        }
      }
    }
  }
}

void Game::updatePlayerDeathState(const InputState& input) {
  TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  GraphicComponent& playerGraphic =
      this->registry->getComponent<GraphicComponent>(this->playerEntityId);
  HealthComponent& playerHealth =
      this->registry->getComponent<HealthComponent>(this->playerEntityId);
  const Position playerCenter = this->playerCenter();

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
    if (distToCorpse <= (RESURRECT_RANGE * RESURRECT_RANGE) && input.resurrectJustPressed) {
      this->isPlayerGhost = false;
      this->hasCorpse = false;
      playerHealth.current = playerHealth.max;
      playerGraphic.color = SDL_Color({240, 240, 240, 255});
      this->playerKnockbackImmunityRemaining = 0.0f;
      this->eventBus->emitFloatingTextEvent(
          FloatingTextEvent{"Resurrected", playerCenter, FloatingTextKind::Info});
    }
  }
}

void Game::updateRegionAndQuestState() {
  const Position playerCenter = this->playerCenter();
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
  const EquipmentComponent& equipment =
      this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
  refreshSecondaryStats(stats, computeEffectivePrimaryStats(stats, equipment, *this->itemDatabase));
}

void Game::applyClassSelection(CharacterClass selectedClass) {
  if (selectedClass == CharacterClass::Any) {
    return;
  }

  InventoryComponent& inventory =
      this->registry->getComponent<InventoryComponent>(this->playerEntityId);
  EquipmentComponent& equipment =
      this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
  StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
  HealthComponent& health = this->registry->getComponent<HealthComponent>(this->playerEntityId);
  ManaComponent& mana = this->registry->getComponent<ManaComponent>(this->playerEntityId);
  const LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
  ClassComponent& playerClass = this->registry->getComponent<ClassComponent>(this->playerEntityId);
  const ClassDefaults defaults = classDefaultsFor(selectedClass);
  playerClass.characterClass = selectedClass;
  stats.baseAttackPower = defaults.baseAttackPower;
  stats.baseArmor = defaults.baseArmor;
  stats.strength = std::max(stats.strength, defaults.strength);
  stats.dexterity = std::max(stats.dexterity, defaults.dexterity);
  stats.intellect = std::max(stats.intellect, defaults.intellect);
  stats.luck = std::max(stats.luck, defaults.luck);
  health.max = std::max(health.max, defaults.health);
  health.current = health.max;
  mana.max = std::max(mana.max, defaults.mana);
  mana.current = mana.max;

  auto grantAndAutoEquip = [&](int itemId) {
    if (itemId <= 0) {
      return;
    }
    ItemInstance starter{itemId};
    if (!inventory.addItem(starter)) {
      return;
    }
    equipItemByIndex(inventory, equipment, *this->itemDatabase, inventory.items.size() - 1,
                     level.level, selectedClass);
  };
  grantAndAutoEquip(defaults.weaponId);
  grantAndAutoEquip(defaults.offhandId);
  refreshSecondaryStats(stats, computeEffectivePrimaryStats(stats, equipment, *this->itemDatabase));

  this->eventBus->emitFloatingTextEvent(
      FloatingTextEvent{"Class selected: " + std::string(className(selectedClass)),
                        this->playerCenter(), FloatingTextKind::Info});
}

void Game::updateClassUnlockAndSelection(const InputState& input) {
  const LevelComponent& level = this->registry->getComponent<LevelComponent>(this->playerEntityId);
  const ClassComponent& playerClass =
      this->registry->getComponent<ClassComponent>(this->playerEntityId);
  if (playerClass.characterClass != CharacterClass::Any) {
    this->classSelectionVisible = false;
    return;
  }
  if (level.level < 10) {
    return;
  }

  if (!this->classUnlockAnnounced) {
    this->classUnlockAnnounced = true;
    this->classSelectionVisible = true;
    this->eventBus->emitFloatingTextEvent(FloatingTextEvent{
        "Class unlocked! Press K to choose.", this->playerCenter(), FloatingTextKind::Info});
  }

  if (input.classMenuJustPressed) {
    this->classSelectionVisible = !this->classSelectionVisible;
  }
  if (!this->classSelectionVisible) {
    return;
  }

  for (std::size_t i = 0; i < kClassUnlockChoices.size(); ++i) {
    if (input.classChoiceJustPressed[i]) {
      applyClassSelection(kClassUnlockChoices[i]);
      this->classSelectionVisible = false;
      return;
    }
  }
}

void Game::updateUiInput(const InputState& input) {
  {
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const LevelComponent& level =
        this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const ClassComponent& characterClass =
        this->registry->getComponent<ClassComponent>(this->playerEntityId);
    this->inventoryUi->handleInput(input.keyboardState, static_cast<int>(input.mouseX),
                                   static_cast<int>(input.mouseY), input.mousePressed, inventory,
                                   equipment, *this->itemDatabase, level, characterClass);
  }
  {
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    this->characterStats->handleInput(static_cast<int>(input.mouseX),
                                      static_cast<int>(input.mouseY), input.mousePressed,
                                      WINDOW_WIDTH, stats, this->inventoryUi->isStatsVisible());
  }
  {
    SkillTreeComponent& skillTree =
        this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
    this->skillTree->handleInput(input.keyboardState, static_cast<int>(input.mouseX),
                                 static_cast<int>(input.mouseY), input.mousePressed, skillTree,
                                 *this->skillTreeDefinition, WINDOW_WIDTH);
  }
  if (this->shopOpen && this->activeNpcId != -1) {
    ShopComponent& shop = this->registry->getComponent<ShopComponent>(this->activeNpcId);
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    this->shopPanel->handleInput(input.mouseX, input.mouseY, input.mouseWheelDelta, input.click,
                                 WINDOW_WIDTH, WINDOW_HEIGHT, this->shopPanelState, shop, inventory,
                                 stats, *this->itemDatabase);
  }
  if (this->questLogVisible && !this->shopOpen && input.mouseWheelDelta != 0.0f) {
    const SDL_FRect panel = this->questLogUi->panelRect(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (pointInRect(input.mouseX, input.mouseY, panel)) {
      this->questLogScroll -= input.mouseWheelDelta * 18.0f;
    }
  }
}

void Game::updateSystems(const std::pair<int, int>& movementInput, float dt) {
  // TODO: Refactor system iteration based on whether update or render is needed.
  for (auto it = this->registry->systemsBegin(); it != this->registry->systemsEnd(); ++it) {
    MovementSystem* movementSystem = dynamic_cast<MovementSystem*>((*it).get());
    if (movementSystem) {
      movementSystem->update(movementInput, dt, *this->map);
    }
    PushbackSystem* pushbackSystem = dynamic_cast<PushbackSystem*>((*it).get());
    if (pushbackSystem) {
      pushbackSystem->update(dt, *this->map);
    }
  }
}

void Game::updateLootPickup(const InputState& input) {
  if (this->isPlayerGhost || !input.pickupJustPressed) {
    return;
  }

  InventoryComponent& inventory =
      this->registry->getComponent<InventoryComponent>(this->playerEntityId);
  const TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  const Position playerCenter = this->playerCenter();
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

  if (closestLootId == -1) {
    return;
  }

  LootComponent& loot = this->registry->getComponent<LootComponent>(closestLootId);
  ItemInstance item{loot.itemId};
  if (!inventory.addItem(item)) {
    return;
  }
  const ItemDef* def = this->itemDatabase->getItem(item.itemId);
  std::string name = def ? def->name : "Item";
  this->eventBus->emitItemPickupEvent(ItemPickupEvent{item.itemId, 1});
  this->eventBus->emitFloatingTextEvent(
      FloatingTextEvent{"Picked up " + name, playerCenter, FloatingTextKind::Info});
  GraphicComponent& graphic = this->registry->getComponent<GraphicComponent>(closestLootId);
  graphic.color = SDL_Color({0, 0, 0, 0});
  TransformComponent& transform = this->registry->getComponent<TransformComponent>(closestLootId);
  transform.position = Position(-1000.0f, -1000.0f);
  this->lootEntityIds.erase(
      std::remove(this->lootEntityIds.begin(), this->lootEntityIds.end(), closestLootId),
      this->lootEntityIds.end());
}

void Game::updateSkillBarAndBuffs(const InputState& input, float dt) {
  SkillBarComponent& skills = this->registry->getComponent<SkillBarComponent>(this->playerEntityId);
  BuffComponent& buffs = this->registry->getComponent<BuffComponent>(this->playerEntityId);
  SkillTreeComponent& skillTree =
      this->registry->getComponent<SkillTreeComponent>(this->playerEntityId);
  const TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  const Position playerCenter = this->playerCenter();
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
    if (input.skillJustPressed[i]) {
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
  }
}

void Game::updateToggles(const InputState& input) {
  if (input.questLogJustPressed) {
    this->questLogVisible = !this->questLogVisible;
  }
  if (input.debugJustPressed) {
    this->showDebugMobRanges = !this->showDebugMobRanges;
  }
}

Position Game::playerCenter() const {
  const TransformComponent& playerTransform =
      this->registry->getComponent<TransformComponent>(this->playerEntityId);
  const CollisionComponent& playerCollision =
      this->registry->getComponent<CollisionComponent>(this->playerEntityId);
  return Position(playerTransform.position.x + (playerCollision.width / 2.0f),
                  playerTransform.position.y + (playerCollision.height / 2.0f));
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
  int entityId =
      createNpcEntity(registry, position, std::move(name), std::move(dialogLine), npcEntityIds);
  registry.registerComponentForEntity<ShopComponent>(
      std::make_unique<ShopComponent>(std::move(stock)), entityId);
  shopNpcIds.push_back(entityId);
  return entityId;
}

int createProjectileEntity(Registry& registry, const Position& position, float velocityX,
                           float velocityY, float range, int sourceEntityId, int targetEntityId,
                           int damage, bool isCrit, float radius, float trailLength,
                           SDL_Color color, std::vector<int>& projectileEntityIds) {
  int entityId = registry.createEntity();
  registry.registerComponentForEntity<TransformComponent>(
      std::make_unique<TransformComponent>(position), entityId);
  auto projectile =
      std::make_unique<ProjectileComponent>(sourceEntityId, targetEntityId, velocityX, velocityY,
                                            range, damage, isCrit, radius, trailLength, color);
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
  this->worldSeed = readWorldSeed();
  this->rng = std::mt19937(deriveSeed(this->worldSeed, kCombatSeedSalt));
  this->lootRng = std::mt19937(deriveSeed(this->worldSeed, kLootSeedSalt));
  logger->info("World seed: {}", this->worldSeed);
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
  this->mobDatabase = std::make_unique<MobDatabase>();
  this->skillDatabase = std::make_unique<SkillDatabase>();
  this->skillTreeDefinition = std::make_unique<SkillTreeDefinition>();
  this->questDatabase = std::make_unique<QuestDatabase>();
  this->questSystem = std::make_unique<QuestSystem>(*this->questDatabase);
  this->eventBus = std::make_unique<EventBus>();
  this->floatingTextSystem = std::make_unique<FloatingTextSystem>(*this->eventBus);
  this->respawnSystem = std::make_unique<RespawnSystem>(
      *this->mobDatabase, deriveSeed(this->worldSeed, kSpawnSeedSalt));
  this->inventoryUi = std::make_unique<Inventory>();
  this->characterStats = std::make_unique<CharacterStats>();
  this->shopPanel = std::make_unique<ShopPanel>();
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
        equipItemByIndex(inventory, equipment, *this->itemDatabase, 0, 1,
                         playerClass.characterClass);
      }
    }
    if (defaults.offhandId > 0) {
      ItemInstance offhandInstance{defaults.offhandId};
      if (inventory.addItem(offhandInstance)) {
        equipItemByIndex(inventory, equipment, *this->itemDatabase, 0, 1,
                         playerClass.characterClass);
      }
    }
    refreshSecondaryStats(stats,
                          computeEffectivePrimaryStats(stats, equipment, *this->itemDatabase));
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
      createShopNpcEntity(*this->registry, npcPosition, "Shopkeeper", "Need supplies? Take a look.",
                          {1, 5, 6, 7, 8, 2, 3, 4}, this->npcEntityIds, this->shopNpcIds);
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
  this->shopPanel->update(dt, this->shopPanelState);
  this->playerHitFlashTimer = std::max(0.0f, this->playerHitFlashTimer - dt);
  this->respawnSystem->update(dt, *this->map, *this->registry, this->mobEntityIds);

  const InputState input = captureInput();
  auto result = std::make_pair(input.moveX, input.moveY);
  updateUiInput(input);

  updateSkillBarAndBuffs(input, dt);
  updateLootPickup(input);
  if (input.questLogJustPressed) {
    this->questLogVisible = !this->questLogVisible;
  }
  if (input.debugJustPressed) {
    this->showDebugMobRanges = !this->showDebugMobRanges;
  }

  updateNpcInteraction(input);
  if (this->shopOpen && this->activeNpcId != -1) {
    ShopComponent& shop = this->registry->getComponent<ShopComponent>(this->activeNpcId);
    InventoryComponent& inventory =
        this->registry->getComponent<InventoryComponent>(this->playerEntityId);
    StatsComponent& stats = this->registry->getComponent<StatsComponent>(this->playerEntityId);
    this->shopPanel->handleInput(input.mouseX, input.mouseY, input.mouseWheelDelta, input.click,
                                 WINDOW_WIDTH, WINDOW_HEIGHT, this->shopPanelState, shop, inventory,
                                 stats, *this->itemDatabase);
  }
  if (this->questLogVisible && !this->shopOpen && input.mouseWheelDelta != 0.0f) {
    const SDL_FRect panel = this->questLogUi->panelRect(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (pointInRect(input.mouseX, input.mouseY, panel)) {
      this->questLogScroll -= input.mouseWheelDelta * 18.0f;
    }
  }

  updateAutoTargetAndFacing(input, dt);
  updatePlayerAttack(dt);

  // spdlog::get("console")->info("Input direction: ({}, {})", result.first,
  //                               result.second);
  updateSystems(result, dt);

  updateMobBehavior(dt);

  updatePlayerDeathState(input);

  updateRegionAndQuestState();
  updateClassUnlockAndSelection(input);
  const Position currentPlayerCenter = playerCenter();
  this->camera->update(currentPlayerCenter);
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
    renderProjectiles(this->renderer, cameraPosition, *this->registry, this->projectileEntityIds);
  }

  { // Quest turn-in markers above NPCs
    const QuestLogComponent& questLog =
        this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
    std::vector<NpcMarkerInfo> npcMarkers;
    for (int npcId : this->npcEntityIds) {
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(npcId);
      const TransformComponent& npcTransform =
          this->registry->getComponent<TransformComponent>(npcId);
      const CollisionComponent& npcCollision =
          this->registry->getComponent<CollisionComponent>(npcId);
      const Position npcCenter = centerForEntity(npcTransform, npcCollision);
      npcMarkers.push_back(NpcMarkerInfo{npc.name, npcCenter});
    }
    const std::vector<Position> markers =
        buildQuestTurnInWorldMarkers(questLog, *this->questDatabase, npcMarkers);
    for (const Position& npcCenter : markers) {
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      drawCircle(this->renderer, Position(npcCenter.x, npcCenter.y - 18.0f), 6.0f, cameraPosition,
                 SDL_Color{255, 220, 80, 200});
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
    const Position playerCenter = this->playerCenter();
    const float markerOffset = (playerCollision.width / 2.0f) + 2.0f;
    const float markerSize = 6.0f;
    const float markerX = playerCenter.x + (this->facingX * markerOffset) - (markerSize / 2.0f);
    const float markerY = playerCenter.y + (this->facingY * markerOffset) - (markerSize / 2.0f);
    SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
    SDL_FRect markerRect = {markerX - cameraPosition.x, markerY - cameraPosition.y, markerSize,
                            markerSize};
    SDL_RenderFillRect(this->renderer, &markerRect);
  }

  { // Facing arc (melee)
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    const Position playerCenter = this->playerCenter();
    const AttackProfile attackProfile = attackProfileForWeapon(equipment, *this->itemDatabase);
    SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
    drawFacingArc(this->renderer, playerCenter, attackProfile.range, this->facingX, this->facingY,
                  attackProfile.halfAngle, cameraPosition, SDL_Color{255, 255, 255, 80});
  }

  { // Auto-attack target ring
    if (this->currentAutoTargetId != -1) {
      const HealthComponent& mobHealth =
          this->registry->getComponent<HealthComponent>(this->currentAutoTargetId);
      if (mobHealth.current > 0 && !this->respawnSystem->isSpawning(this->currentAutoTargetId)) {
        const TransformComponent& mobTransform =
            this->registry->getComponent<TransformComponent>(this->currentAutoTargetId);
        const CollisionComponent& mobCollision =
            this->registry->getComponent<CollisionComponent>(this->currentAutoTargetId);
        const Position mobCenter = centerForEntity(mobTransform, mobCollision);
        SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
        drawCircle(this->renderer, mobCenter, (mobCollision.width / 2.0f) + 6.0f, cameraPosition,
                   SDL_Color{255, 230, 120, 220});
        drawCircle(this->renderer, mobCenter, (mobCollision.width / 2.0f) + 3.0f, cameraPosition,
                   SDL_Color{255, 255, 255, 140});
      }
    }
  }

  { // Loot labels and pickup prompt
    if (!this->isPlayerGhost) {
      const Position playerCenter = this->playerCenter();
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
          SDL_Texture* promptTexture = SDL_CreateTextureFromSurface(this->renderer, promptSurface);
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
      SDL_FRect promptRect = {12.0f, WINDOW_HEIGHT - 140.0f, static_cast<float>(promptSurface->w),
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
      this->shopPanel->render(this->renderer, this->font, WINDOW_WIDTH, WINDOW_HEIGHT, mouseX,
                              mouseY, npc.name, this->shopPanelState, shop, inventory, stats,
                              *this->itemDatabase);
    }
  }

  { // NPC dialog
    if (this->activeNpcId != -1 && !this->shopOpen) {
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(this->activeNpcId);
      const std::string title = npc.name;
      const QuestLogComponent& questLog =
          this->registry->getComponent<QuestLogComponent>(this->playerEntityId);
      const LevelComponent& level =
          this->registry->getComponent<LevelComponent>(this->playerEntityId);
      const std::vector<QuestEntry> entries =
          buildNpcQuestEntries(npc.name, *this->questSystem, *this->questDatabase, questLog, level);
      const std::string dialogText =
          buildNpcDialogText(npc.dialogLine, entries, this->activeNpcQuestSelection);
      renderNpcDialog(this->renderer, this->font, title, dialogText, WINDOW_WIDTH, WINDOW_HEIGHT,
                      this->npcDialogScroll);
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
                       playerTransform.position.y - cameraPosition.y + playerCollision.height +
                           4.0f,
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
    const ClassComponent& playerClass =
        this->registry->getComponent<ClassComponent>(this->playerEntityId);
    const EquipmentComponent& equipment =
        this->registry->getComponent<EquipmentComponent>(this->playerEntityId);
    int attackPower =
        computeAttackPower(stats, equipment, *this->itemDatabase, playerClass.characterClass);
    const char* powerLabel = (playerClass.characterClass == CharacterClass::Mage) ? "SP" : "AP";
    hud << "HP " << health.current << "/" << health.max << "  MP " << mana.current << "/"
        << mana.max << "  " << powerLabel << " " << attackPower << "  LV " << level.level;
    if (level.level >= PLAYER_LEVEL_CAP) {
      hud << " (MAX)";
    } else {
      hud << " XP " << level.experience << "/" << level.nextLevelExperience;
    }
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

  { // Class unlock hint and selection overlay
    const LevelComponent& level =
        this->registry->getComponent<LevelComponent>(this->playerEntityId);
    const ClassComponent& playerClass =
        this->registry->getComponent<ClassComponent>(this->playerEntityId);
    if (playerClass.characterClass == CharacterClass::Any && level.level >= 10) {
      SDL_Color hintColor = {255, 240, 200, 255};
      const std::string hint = "Class unlock available: Press K";
      SDL_Surface* hintSurface =
          TTF_RenderText_Solid(this->font, hint.c_str(), hint.length(), hintColor);
      SDL_Texture* hintTexture = SDL_CreateTextureFromSurface(this->renderer, hintSurface);
      SDL_FRect hintRect = {12.0f, 40.0f, static_cast<float>(hintSurface->w),
                            static_cast<float>(hintSurface->h)};
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 170);
      SDL_FRect hintBg = {hintRect.x - 6.0f, hintRect.y - 4.0f, hintRect.w + 12.0f,
                          hintRect.h + 8.0f};
      SDL_RenderFillRect(this->renderer, &hintBg);
      SDL_RenderTexture(this->renderer, hintTexture, nullptr, &hintRect);
      SDL_DestroySurface(hintSurface);
      SDL_DestroyTexture(hintTexture);
    }

    if (this->classSelectionVisible && playerClass.characterClass == CharacterClass::Any &&
        level.level >= 10) {
      SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 150);
      SDL_FRect dimRect = {0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH),
                           static_cast<float>(WINDOW_HEIGHT)};
      SDL_RenderFillRect(this->renderer, &dimRect);

      SDL_FRect panel = {WINDOW_WIDTH * 0.5f - 180.0f, WINDOW_HEIGHT * 0.5f - 110.0f, 360.0f,
                         220.0f};
      SDL_SetRenderDrawColor(this->renderer, 16, 20, 30, 235);
      SDL_RenderFillRect(this->renderer, &panel);
      SDL_SetRenderDrawColor(this->renderer, 240, 240, 240, 200);
      SDL_RenderRect(this->renderer, &panel);

      auto renderOverlayText = [&](const std::string& text, float x, float y, SDL_Color color) {
        SDL_Surface* lineSurface =
            TTF_RenderText_Solid(this->font, text.c_str(), text.length(), color);
        SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(this->renderer, lineSurface);
        SDL_FRect lineRect = {x, y, static_cast<float>(lineSurface->w),
                              static_cast<float>(lineSurface->h)};
        SDL_RenderTexture(this->renderer, lineTexture, nullptr, &lineRect);
        SDL_DestroySurface(lineSurface);
        SDL_DestroyTexture(lineTexture);
      };

      renderOverlayText("Choose your class", panel.x + 16.0f, panel.y + 14.0f,
                        SDL_Color{255, 255, 255, 255});
      renderOverlayText("Your choice grants class starter gear.", panel.x + 16.0f, panel.y + 36.0f,
                        SDL_Color{200, 220, 255, 255});

      float optionY = panel.y + 70.0f;
      for (CharacterClass option : kClassUnlockChoices) {
        renderOverlayText(classUnlockLabel(option), panel.x + 24.0f, optionY,
                          SDL_Color{255, 245, 210, 255});
        optionY += 28.0f;
      }
      renderOverlayText("Press K to close", panel.x + 16.0f, panel.y + panel.h - 24.0f,
                        SDL_Color{180, 180, 180, 255});
    }
  }

  if (this->isPlayerGhost && this->hasCorpse) {
    const CollisionComponent& playerCollision =
        this->registry->getComponent<CollisionComponent>(this->playerEntityId);
    const Position playerCenter = this->playerCenter();
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
    std::vector<NpcMarkerInfo> npcMarkers;
    for (int npcId : this->npcEntityIds) {
      const NpcComponent& npc = this->registry->getComponent<NpcComponent>(npcId);
      const TransformComponent& npcTransform =
          this->registry->getComponent<TransformComponent>(npcId);
      npcMarkers.push_back(NpcMarkerInfo{npc.name, npcTransform.position});
    }
    const std::vector<MinimapMarker> markers =
        buildQuestMinimapMarkers(questLog, *this->questDatabase, *this->map, npcMarkers);
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
    const EffectivePrimaryStats effectiveStats =
        computeEffectivePrimaryStats(stats, equipment, *this->itemDatabase);
    const int attackPower =
        computeAttackPower(stats, equipment, *this->itemDatabase, playerClass.characterClass);
    this->characterStats->render(this->renderer, this->font, WINDOW_WIDTH, health, mana, level,
                                 attackPower, className(playerClass.characterClass),
                                 effectiveStats.strength, stats.gold, effectiveStats.dexterity,
                                 effectiveStats.intellect, effectiveStats.luck, stats.unspentPoints,
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
