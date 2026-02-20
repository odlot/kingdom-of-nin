#include "mobs/mob_database.h"
#include <cstdlib>
#include <iostream>
#include <random>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    failures += 1;
  }
}

bool sameDropOptions(const EquipmentDropGenerationOptions& a,
                     const EquipmentDropGenerationOptions& b) {
  return a.weaponWeight == b.weaponWeight && a.armorWeight == b.armorWeight &&
         a.commonWeight == b.commonWeight && a.rareWeight == b.rareWeight &&
         a.epicWeight == b.epicWeight;
}
} // namespace

int main() {
  MobDatabase database;

  {
    expect(database.get(MobType::Goblin) != nullptr, "has goblin archetype");
    expect(database.get(MobType::GoblinArcher) != nullptr, "has goblin archer archetype");
    expect(database.get(MobType::GoblinBrute) != nullptr, "has goblin brute archetype");
  }

  {
    const MobResolvedStats level1 = database.resolveStats(MobType::Goblin, 1);
    const MobResolvedStats level10 = database.resolveStats(MobType::Goblin, 10);
    expect(level10.maxHealth > level1.maxHealth, "mob health scales by level");
    expect(level10.experience > level1.experience, "mob experience scales by level");
    expect(level10.attackDamage >= level1.attackDamage, "mob damage scales by level");
  }

  {
    std::mt19937 rngA(42);
    std::mt19937 rngB(42);
    for (int i = 0; i < 60; ++i) {
      EquipmentDropGenerationOptions optionsA;
      EquipmentDropGenerationOptions optionsB;
      const bool dropA = database.rollEquipmentDrop(MobType::GoblinBrute, rngA, optionsA);
      const bool dropB = database.rollEquipmentDrop(MobType::GoblinBrute, rngB, optionsB);
      expect(dropA == dropB, "drop roll is deterministic for fixed seed");
      if (dropA && dropB) {
        expect(sameDropOptions(optionsA, optionsB), "drop options are deterministic");
      }
    }
  }

  if (failures > 0) {
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
  }
  std::cout << "All mob database tests passed.\n";
  return EXIT_SUCCESS;
}
