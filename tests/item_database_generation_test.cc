#include "items/item_database.h"
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
} // namespace

int main() {
  ItemDatabase database;
  std::mt19937 rng(1337);

  {
    const int itemId = database.generateEquipmentDrop(75, CharacterClass::Warrior, rng);
    const ItemDef* def = database.getItem(itemId);
    expect(def != nullptr, "generated item exists");
    if (def) {
      expect(def->requiredLevel == 60, "generated level is clamped to level cap");
      expect(def->allowedClasses.count(CharacterClass::Warrior) > 0,
             "generated item is usable by preferred class");
      expect(def->stats.armor > 0 || def->slot == ItemSlot::Weapon,
             "generated item has armor when applicable");
      expect(def->stats.strength + def->stats.dexterity + def->stats.intellect + def->stats.luck >
                 0,
             "generated item has at least one primary stat bonus");
    }
  }

  {
    int rareOrBetter = 0;
    for (int i = 0; i < 80; ++i) {
      const int itemId = database.generateEquipmentDrop(20, CharacterClass::Rogue, rng);
      const ItemDef* def = database.getItem(itemId);
      expect(def != nullptr, "bulk-generated item exists");
      if (!def) {
        continue;
      }
      expect(def->requiredLevel == 20, "generated item preserves valid target level");
      if (def->rarity != ItemRarity::Common) {
        rareOrBetter += 1;
      }
    }
    expect(rareOrBetter > 0, "rarity roll can generate rare or epic items");
  }

  if (failures > 0) {
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
  }
  std::cout << "All item generation tests passed.\n";
  return EXIT_SUCCESS;
}
