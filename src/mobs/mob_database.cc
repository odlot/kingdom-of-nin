#include "mobs/mob_database.h"

#include <algorithm>

namespace {
constexpr int MOB_LEVEL_CAP = 60;

int clampedWeight(int value) {
  return std::max(0, value);
}
} // namespace

MobDatabase::MobDatabase() {
  this->archetypes = {{
      {MobType::Goblin, "Goblin", SDL_Color{40, 200, 80, 255}, 20, 5, 20, 3, 5, 1, 1.2f, 36.0f,
       60.0f, 160.0f, 220.0f, MobLootTable{25, EquipmentDropGenerationOptions{30, 70, 80, 18, 2}}},
      {MobType::GoblinArcher, "Goblin Archer", SDL_Color{60, 160, 220, 255}, 16, 4, 25, 4, 4, 1,
       1.0f, 80.0f, 70.0f, 180.0f, 240.0f,
       MobLootTable{20, EquipmentDropGenerationOptions{45, 55, 65, 30, 5}}},
      {MobType::GoblinBrute, "Goblin Brute", SDL_Color{120, 180, 60, 255}, 30, 7, 35, 5, 7, 2, 1.6f,
       36.0f, 45.0f, 140.0f, 200.0f,
       MobLootTable{15, EquipmentDropGenerationOptions{50, 50, 55, 33, 12}}},
  }};
}

const MobArchetype* MobDatabase::get(MobType type) const {
  for (const MobArchetype& archetype : this->archetypes) {
    if (archetype.type == type) {
      return &archetype;
    }
  }
  return nullptr;
}

const MobArchetype& MobDatabase::randomArchetype(std::mt19937& rng) const {
  std::uniform_int_distribution<std::size_t> dist(0, this->archetypes.size() - 1);
  return this->archetypes[dist(rng)];
}

MobResolvedStats MobDatabase::resolveStats(MobType type, int level) const {
  const MobArchetype* archetype = get(type);
  if (!archetype) {
    return MobResolvedStats{};
  }
  const int clampedLevel = std::clamp(level, 1, MOB_LEVEL_CAP);
  const int levelOffset = clampedLevel - 1;
  MobResolvedStats resolved;
  resolved.level = clampedLevel;
  resolved.color = archetype->color;
  resolved.maxHealth =
      std::max(1, archetype->baseHealth + (levelOffset * archetype->healthPerLevel));
  resolved.experience =
      std::max(1, archetype->baseExperience + (levelOffset * archetype->experiencePerLevel));
  resolved.attackDamage =
      std::max(1, archetype->baseAttackDamage + (levelOffset * archetype->attackDamagePerLevel));
  resolved.attackCooldown = archetype->attackCooldown;
  resolved.attackRange = archetype->attackRange;
  resolved.speed = archetype->speed;
  resolved.aggroRange = archetype->aggroRange;
  resolved.leashRange = archetype->leashRange;
  return resolved;
}

bool MobDatabase::rollEquipmentDrop(MobType type, std::mt19937& rng,
                                    EquipmentDropGenerationOptions& outOptions) const {
  const MobArchetype* archetype = get(type);
  if (!archetype) {
    return false;
  }
  const MobLootTable& table = archetype->lootTable;
  const EquipmentDropGenerationOptions& drop = table.equipmentDropOptions;
  const int rarityWeight = clampedWeight(drop.commonWeight) + clampedWeight(drop.rareWeight) +
                           clampedWeight(drop.epicWeight);
  if (rarityWeight <= 0) {
    return false;
  }
  const int noDropWeight = clampedWeight(table.noDropWeight);
  const int totalWeight = noDropWeight + rarityWeight;
  if (totalWeight <= 0) {
    return false;
  }
  std::uniform_int_distribution<int> dist(1, totalWeight);
  const int roll = dist(rng);
  if (roll <= noDropWeight) {
    return false;
  }
  outOptions = drop;
  return true;
}
