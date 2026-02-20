#pragma once

#include <random>
#include <vector>

#include "SDL3/SDL.h"
#include "ecs/component/mob_component.h"
#include "items/item_database.h"

struct MobLootTable {
  int noDropWeight = 0;
  EquipmentDropGenerationOptions equipmentDropOptions;
};

struct MobArchetype {
  MobType type = MobType::Goblin;
  const char* name = "";
  SDL_Color color = {255, 255, 255, 255};
  int baseHealth = 1;
  int healthPerLevel = 1;
  int baseExperience = 1;
  int experiencePerLevel = 0;
  int baseAttackDamage = 1;
  int attackDamagePerLevel = 0;
  float attackCooldown = 1.0f;
  float attackRange = 32.0f;
  float speed = 60.0f;
  float aggroRange = 120.0f;
  float leashRange = 180.0f;
  int minSpawnLevel = 1;
  int maxSpawnLevel = 60;
  int minSpawnTier = 0;
  int maxSpawnTier = 11;
  int spawnWeight = 10;
  MobLootTable lootTable;
};

struct MobResolvedStats {
  int level = 1;
  SDL_Color color = {255, 255, 255, 255};
  int maxHealth = 1;
  int experience = 1;
  int attackDamage = 1;
  float attackCooldown = 1.0f;
  float attackRange = 32.0f;
  float speed = 60.0f;
  float aggroRange = 120.0f;
  float leashRange = 180.0f;
};

class MobDatabase {
public:
  MobDatabase();

  const MobArchetype* get(MobType type) const;
  const std::vector<MobArchetype>& allArchetypes() const { return archetypes; }
  const MobArchetype& randomArchetype(std::mt19937& rng) const;
  const MobArchetype& randomArchetypeForBand(int spawnTier, int level, std::mt19937& rng) const;
  MobResolvedStats resolveStats(MobType type, int level) const;
  bool rollEquipmentDrop(MobType type, std::mt19937& rng,
                         EquipmentDropGenerationOptions& outOptions) const;

private:
  std::vector<MobArchetype> archetypes;
};
