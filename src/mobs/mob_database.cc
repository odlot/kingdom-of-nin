#include "mobs/mob_database.h"

#include <algorithm>

namespace {
constexpr int MOB_LEVEL_CAP = 60;

int clampedWeight(int value) {
  return std::max(0, value);
}
} // namespace

MobDatabase::MobDatabase() {
  this->archetypes = {
      {MobType::Goblin,
       "Goblin",
       SDL_Color{40, 200, 80, 255},
       20,
       5,
       20,
       3,
       5,
       1,
       1.2f,
       36.0f,
       60.0f,
       160.0f,
       220.0f,
       1,
       20,
       0,
       3,
       30,
       MobLootTable{25, EquipmentDropGenerationOptions{30, 70, 80, 18, 2}}},
      {MobType::GoblinArcher,
       "Goblin Archer",
       SDL_Color{60, 160, 220, 255},
       16,
       4,
       25,
       4,
       4,
       1,
       1.0f,
       80.0f,
       70.0f,
       180.0f,
       240.0f,
       4,
       24,
       1,
       4,
       20,
       MobLootTable{20, EquipmentDropGenerationOptions{45, 55, 65, 30, 5}}},
      {MobType::GoblinBrute,
       "Goblin Brute",
       SDL_Color{120, 180, 60, 255},
       30,
       7,
       35,
       5,
       7,
       2,
       1.6f,
       36.0f,
       45.0f,
       140.0f,
       200.0f,
       8,
       30,
       2,
       5,
       14,
       MobLootTable{15, EquipmentDropGenerationOptions{50, 50, 55, 33, 12}}},
      {MobType::Skeleton,
       "Skeleton",
       SDL_Color{206, 206, 198, 255},
       24,
       6,
       24,
       4,
       6,
       1,
       1.25f,
       40.0f,
       55.0f,
       165.0f,
       230.0f,
       12,
       34,
       2,
       6,
       22,
       MobLootTable{22, EquipmentDropGenerationOptions{35, 65, 72, 24, 4}}},
      {MobType::SkeletonArcher,
       "Skeleton Archer",
       SDL_Color{170, 180, 196, 255},
       20,
       5,
       27,
       4,
       5,
       1,
       1.1f,
       90.0f,
       62.0f,
       185.0f,
       250.0f,
       16,
       38,
       3,
       7,
       16,
       MobLootTable{18, EquipmentDropGenerationOptions{50, 50, 60, 32, 8}}},
      {MobType::Necromancer,
       "Necromancer",
       SDL_Color{125, 110, 180, 255},
       18,
       4,
       34,
       5,
       8,
       2,
       1.8f,
       100.0f,
       52.0f,
       190.0f,
       260.0f,
       28,
       52,
       5,
       9,
       10,
       MobLootTable{14, EquipmentDropGenerationOptions{45, 55, 45, 37, 18}}},
      {MobType::Wolf,
       "Wolf",
       SDL_Color{130, 130, 130, 255},
       18,
       5,
       22,
       3,
       5,
       1,
       1.0f,
       32.0f,
       85.0f,
       170.0f,
       220.0f,
       6,
       28,
       1,
       5,
       26,
       MobLootTable{28, EquipmentDropGenerationOptions{20, 80, 82, 16, 2}}},
      {MobType::DireWolf,
       "Dire Wolf",
       SDL_Color{90, 95, 105, 255},
       28,
       6,
       30,
       4,
       7,
       2,
       1.1f,
       34.0f,
       95.0f,
       185.0f,
       240.0f,
       18,
       42,
       3,
       7,
       14,
       MobLootTable{20, EquipmentDropGenerationOptions{25, 75, 75, 22, 3}}},
      {MobType::Bandit,
       "Bandit",
       SDL_Color{186, 152, 110, 255},
       26,
       6,
       32,
       4,
       7,
       1,
       1.2f,
       38.0f,
       65.0f,
       175.0f,
       235.0f,
       24,
       46,
       4,
       8,
       20,
       MobLootTable{20, EquipmentDropGenerationOptions{40, 60, 70, 25, 5}}},
      {MobType::BanditArcher,
       "Bandit Archer",
       SDL_Color{168, 130, 90, 255},
       22,
       5,
       34,
       4,
       6,
       1,
       1.0f,
       92.0f,
       68.0f,
       185.0f,
       245.0f,
       28,
       50,
       5,
       9,
       15,
       MobLootTable{18, EquipmentDropGenerationOptions{55, 45, 60, 32, 8}}},
      {MobType::BanditBruiser,
       "Bandit Bruiser",
       SDL_Color{148, 110, 86, 255},
       36,
       8,
       40,
       6,
       9,
       2,
       1.5f,
       36.0f,
       58.0f,
       170.0f,
       230.0f,
       32,
       55,
       6,
       10,
       10,
       MobLootTable{12, EquipmentDropGenerationOptions{45, 55, 50, 35, 15}}},
      {MobType::Slime,
       "Slime",
       SDL_Color{96, 212, 186, 255},
       34,
       9,
       38,
       6,
       8,
       2,
       1.7f,
       34.0f,
       40.0f,
       135.0f,
       190.0f,
       30,
       55,
       5,
       10,
       12,
       MobLootTable{35, EquipmentDropGenerationOptions{10, 90, 88, 11, 1}}},
      {MobType::ArcaneWisp,
       "Arcane Wisp",
       SDL_Color{130, 182, 255, 255},
       24,
       5,
       42,
       6,
       9,
       2,
       1.3f,
       96.0f,
       78.0f,
       200.0f,
       260.0f,
       36,
       60,
       7,
       11,
       12,
       MobLootTable{24, EquipmentDropGenerationOptions{50, 50, 58, 33, 9}}},
      {MobType::ArcaneSentinel,
       "Arcane Sentinel",
       SDL_Color{120, 132, 220, 255},
       42,
       10,
       48,
       7,
       11,
       2,
       1.7f,
       88.0f,
       48.0f,
       190.0f,
       255.0f,
       44,
       60,
       8,
       11,
       10,
       MobLootTable{14, EquipmentDropGenerationOptions{40, 60, 48, 35, 17}}},
      {MobType::Ogre,
       "Ogre",
       SDL_Color{90, 170, 120, 255},
       52,
       12,
       56,
       8,
       13,
       3,
       1.9f,
       40.0f,
       42.0f,
       165.0f,
       220.0f,
       40,
       60,
       7,
       11,
       8,
       MobLootTable{10, EquipmentDropGenerationOptions{50, 50, 42, 38, 20}}},
  };
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

const MobArchetype& MobDatabase::randomArchetypeForBand(int spawnTier, int level,
                                                        std::mt19937& rng) const {
  std::vector<const MobArchetype*> candidates;
  std::vector<int> weights;
  const int clampedLevel = std::clamp(level, 1, MOB_LEVEL_CAP);
  for (const MobArchetype& archetype : this->archetypes) {
    if (spawnTier < archetype.minSpawnTier || spawnTier > archetype.maxSpawnTier) {
      continue;
    }
    if (clampedLevel < archetype.minSpawnLevel || clampedLevel > archetype.maxSpawnLevel) {
      continue;
    }
    candidates.push_back(&archetype);
    weights.push_back(std::max(1, archetype.spawnWeight));
  }
  if (candidates.empty()) {
    return randomArchetype(rng);
  }
  std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
  return *candidates[dist(rng)];
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
