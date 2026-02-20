#include "items/item_database.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int ITEM_LEVEL_CAP = 60;
constexpr std::array<ItemSlot, 10> kDropSlots = {
    ItemSlot::Weapon, ItemSlot::Weapon, ItemSlot::Shield, ItemSlot::Chest,     ItemSlot::Chest,
    ItemSlot::Pants,  ItemSlot::Pants,  ItemSlot::Boots,  ItemSlot::Shoulders, ItemSlot::Cape};

constexpr std::array<const char*, 5> kCommonPrefixes = {"Worn", "Sturdy", "Reliable", "Fine",
                                                        "Traveler's"};
constexpr std::array<const char*, 4> kRarePrefixes = {"Runed", "Valiant", "Stormforged",
                                                      "Champion's"};
constexpr std::array<const char*, 4> kEpicPrefixes = {"Mythic", "Ancient", "Dragonforged",
                                                      "Legendary"};

const char* slotName(ItemSlot slot) {
  switch (slot) {
  case ItemSlot::Weapon:
    return "Weapon";
  case ItemSlot::Shield:
    return "Shield";
  case ItemSlot::Shoulders:
    return "Shoulders";
  case ItemSlot::Chest:
    return "Chest";
  case ItemSlot::Pants:
    return "Pants";
  case ItemSlot::Boots:
    return "Boots";
  case ItemSlot::Cape:
    return "Cape";
  }
  return "Gear";
}

const char* classLabel(CharacterClass characterClass) {
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

ItemRarity rollRarity(std::mt19937& rng) {
  std::uniform_int_distribution<int> dist(1, 100);
  const int roll = dist(rng);
  if (roll <= 70) {
    return ItemRarity::Common;
  }
  if (roll <= 95) {
    return ItemRarity::Rare;
  }
  return ItemRarity::Epic;
}

float rarityMultiplier(ItemRarity rarity) {
  switch (rarity) {
  case ItemRarity::Common:
    return 1.0f;
  case ItemRarity::Rare:
    return 1.35f;
  case ItemRarity::Epic:
    return 1.8f;
  }
  return 1.0f;
}

int rarityFlatBonus(ItemRarity rarity, int level) {
  switch (rarity) {
  case ItemRarity::Common:
    return 0;
  case ItemRarity::Rare:
    return std::max(1, level / 7);
  case ItemRarity::Epic:
    return std::max(2, level / 4);
  }
  return 0;
}

float rollVariance(ItemRarity rarity, std::mt19937& rng) {
  switch (rarity) {
  case ItemRarity::Common: {
    std::uniform_real_distribution<float> dist(0.88f, 1.12f);
    return dist(rng);
  }
  case ItemRarity::Rare: {
    std::uniform_real_distribution<float> dist(0.92f, 1.18f);
    return dist(rng);
  }
  case ItemRarity::Epic: {
    std::uniform_real_distribution<float> dist(0.96f, 1.25f);
    return dist(rng);
  }
  }
  return 1.0f;
}

int focusStatIndex(CharacterClass characterClass, std::mt19937& rng) {
  switch (characterClass) {
  case CharacterClass::Warrior:
    return 0;
  case CharacterClass::Archer:
    return 1;
  case CharacterClass::Mage:
    return 2;
  case CharacterClass::Rogue:
    return 3;
  case CharacterClass::Any:
    break;
  }
  std::uniform_int_distribution<int> dist(0, 3);
  return dist(rng);
}

void addPrimaryStatByIndex(ItemStats& stats, int statIndex, int amount) {
  switch (statIndex) {
  case 0:
    stats.strength += amount;
    break;
  case 1:
    stats.dexterity += amount;
    break;
  case 2:
    stats.intellect += amount;
    break;
  case 3:
    stats.luck += amount;
    break;
  default:
    break;
  }
}

WeaponType weaponTypeForClass(CharacterClass characterClass, std::mt19937& rng) {
  switch (characterClass) {
  case CharacterClass::Warrior: {
    const std::array<WeaponType, 3> options = {WeaponType::OneHandedSword, WeaponType::Polearm,
                                               WeaponType::Spear};
    std::uniform_int_distribution<std::size_t> dist(0, options.size() - 1);
    return options[dist(rng)];
  }
  case CharacterClass::Mage:
    return WeaponType::Wand;
  case CharacterClass::Archer:
    return WeaponType::Bow;
  case CharacterClass::Rogue:
    return WeaponType::Dagger;
  case CharacterClass::Any:
    break;
  }
  return WeaponType::OneHandedSword;
}

const char* randomPrefix(ItemRarity rarity, std::mt19937& rng) {
  switch (rarity) {
  case ItemRarity::Common: {
    std::uniform_int_distribution<std::size_t> dist(0, kCommonPrefixes.size() - 1);
    return kCommonPrefixes[dist(rng)];
  }
  case ItemRarity::Rare: {
    std::uniform_int_distribution<std::size_t> dist(0, kRarePrefixes.size() - 1);
    return kRarePrefixes[dist(rng)];
  }
  case ItemRarity::Epic: {
    std::uniform_int_distribution<std::size_t> dist(0, kEpicPrefixes.size() - 1);
    return kEpicPrefixes[dist(rng)];
  }
  }
  return "Gear";
}

void applyWeaponProjectileDefaults(ItemDef& def) {
  switch (def.weaponType) {
  case WeaponType::Bow:
    def.projectile.speed = 220.0f;
    def.projectile.radius = 9.0f;
    def.projectile.trailLength = 22.0f;
    break;
  case WeaponType::Wand:
    def.projectile.speed = 200.0f;
    def.projectile.radius = 10.0f;
    def.projectile.trailLength = 26.0f;
    break;
  default:
    break;
  }
}
} // namespace

ItemDatabase::ItemDatabase() {
  ItemDef basicSword;
  basicSword.id = 1;
  basicSword.name = "Basic Sword";
  basicSword.slot = ItemSlot::Weapon;
  basicSword.requiredLevel = 1;
  basicSword.allowedClasses = {CharacterClass::Any};
  basicSword.weaponType = WeaponType::OneHandedSword;
  basicSword.stats.attackPower = 5;
  basicSword.price = 20;
  addItem(basicSword);

  ItemDef basicSpear;
  basicSpear.id = 5;
  basicSpear.name = "Basic Spear";
  basicSpear.slot = ItemSlot::Weapon;
  basicSpear.requiredLevel = 1;
  basicSpear.allowedClasses = {CharacterClass::Any};
  basicSpear.weaponType = WeaponType::Spear;
  basicSpear.stats.attackPower = 6;
  basicSpear.price = 24;
  addItem(basicSpear);

  ItemDef basicBow;
  basicBow.id = 6;
  basicBow.name = "Basic Bow";
  basicBow.slot = ItemSlot::Weapon;
  basicBow.requiredLevel = 1;
  basicBow.allowedClasses = {CharacterClass::Archer};
  basicBow.weaponType = WeaponType::Bow;
  basicBow.stats.attackPower = 5;
  basicBow.projectile.speed = 220.0f;
  basicBow.projectile.radius = 9.0f;
  basicBow.projectile.trailLength = 22.0f;
  basicBow.price = 30;
  addItem(basicBow);

  ItemDef basicWand;
  basicWand.id = 7;
  basicWand.name = "Basic Wand";
  basicWand.slot = ItemSlot::Weapon;
  basicWand.requiredLevel = 1;
  basicWand.allowedClasses = {CharacterClass::Mage};
  basicWand.weaponType = WeaponType::Wand;
  basicWand.stats.attackPower = 4;
  basicWand.projectile.speed = 200.0f;
  basicWand.projectile.radius = 10.0f;
  basicWand.projectile.trailLength = 26.0f;
  basicWand.price = 28;
  addItem(basicWand);

  ItemDef basicDagger;
  basicDagger.id = 8;
  basicDagger.name = "Basic Dagger";
  basicDagger.slot = ItemSlot::Weapon;
  basicDagger.requiredLevel = 1;
  basicDagger.allowedClasses = {CharacterClass::Rogue};
  basicDagger.weaponType = WeaponType::Dagger;
  basicDagger.stats.attackPower = 4;
  basicDagger.price = 18;
  addItem(basicDagger);

  ItemDef basicShield;
  basicShield.id = 2;
  basicShield.name = "Basic Shield";
  basicShield.slot = ItemSlot::Shield;
  basicShield.requiredLevel = 1;
  basicShield.allowedClasses = {CharacterClass::Any};
  basicShield.stats.armor = 3;
  basicShield.price = 16;
  addItem(basicShield);

  ItemDef basicBoots;
  basicBoots.id = 3;
  basicBoots.name = "Basic Boots";
  basicBoots.slot = ItemSlot::Boots;
  basicBoots.requiredLevel = 1;
  basicBoots.allowedClasses = {CharacterClass::Any};
  basicBoots.stats.armor = 1;
  basicBoots.price = 12;
  addItem(basicBoots);

  ItemDef basicChest;
  basicChest.id = 4;
  basicChest.name = "Basic Tunic";
  basicChest.slot = ItemSlot::Chest;
  basicChest.requiredLevel = 1;
  basicChest.allowedClasses = {CharacterClass::Any};
  basicChest.stats.armor = 2;
  basicChest.price = 22;
  addItem(basicChest);
}

const ItemDef* ItemDatabase::getItem(int id) const {
  auto it = items.find(id);
  if (it == items.end()) {
    return nullptr;
  }
  return &it->second;
}

void ItemDatabase::addItem(ItemDef def) {
  items.emplace(def.id, std::move(def));
}

int ItemDatabase::generateEquipmentDrop(int targetLevel, CharacterClass preferredClass,
                                        std::mt19937& rng) {
  const int level = std::clamp(targetLevel, 1, ITEM_LEVEL_CAP);
  const ItemRarity rarity = rollRarity(rng);
  std::uniform_int_distribution<std::size_t> slotDist(0, kDropSlots.size() - 1);
  const ItemSlot slot = kDropSlots[slotDist(rng)];

  ItemDef generated;
  generated.id = this->nextGeneratedItemId++;
  generated.slot = slot;
  generated.rarity = rarity;
  generated.requiredLevel = level;
  generated.allowedClasses = (preferredClass == CharacterClass::Any)
                                 ? std::unordered_set<CharacterClass>{CharacterClass::Any}
                                 : std::unordered_set<CharacterClass>{preferredClass};

  const float multiplier = rarityMultiplier(rarity);
  const float variance = rollVariance(rarity, rng);
  const int flatBonus = rarityFlatBonus(rarity, level);
  if (slot == ItemSlot::Weapon) {
    generated.weaponType = weaponTypeForClass(preferredClass, rng);
    const float baseAp = 4.0f + (static_cast<float>(level) * 1.9f);
    generated.stats.attackPower =
        std::max(1, static_cast<int>(std::round(baseAp * multiplier * variance)) + flatBonus);
    applyWeaponProjectileDefaults(generated);
  } else if (slot == ItemSlot::Shield) {
    const float baseArmor = 3.0f + (static_cast<float>(level) * 1.4f);
    generated.stats.armor =
        std::max(1, static_cast<int>(std::round(baseArmor * multiplier * variance)) + flatBonus);
  } else {
    const float baseArmor = 2.0f + (static_cast<float>(level) * 1.1f);
    generated.stats.armor =
        std::max(1, static_cast<int>(std::round(baseArmor * multiplier * variance)) + flatBonus);
  }

  const int focusedStat = focusStatIndex(preferredClass, rng);
  const float primaryBase =
      (slot == ItemSlot::Weapon) ? (2.0f + (0.6f * level)) : (1.0f + (0.45f * level));
  const int primaryBonus =
      std::max(1, static_cast<int>(std::round(primaryBase * multiplier * variance)));
  addPrimaryStatByIndex(generated.stats, focusedStat, primaryBonus);

  if (rarity != ItemRarity::Common) {
    std::vector<int> candidateStats = {0, 1, 2, 3};
    candidateStats.erase(std::remove(candidateStats.begin(), candidateStats.end(), focusedStat),
                         candidateStats.end());
    std::shuffle(candidateStats.begin(), candidateStats.end(), rng);

    const int extraCount = (rarity == ItemRarity::Rare) ? 1 : 2;
    for (int i = 0; i < extraCount; ++i) {
      const int statIndex = candidateStats[static_cast<std::size_t>(i)];
      const int extraBonus =
          std::max(1, static_cast<int>(std::round((0.15f * level) * multiplier)) + flatBonus);
      addPrimaryStatByIndex(generated.stats, statIndex, extraBonus);
    }
  }

  const std::string rarityToken = itemRarityName(rarity);
  const std::string classToken = classLabel(preferredClass);
  generated.name = std::string(randomPrefix(rarity, rng)) + " " + classToken + " " +
                   slotName(slot) + " (" + rarityToken + " L" + std::to_string(level) + ")";
  generated.price = std::max(10, (level * 3) + (flatBonus * 5) + (generated.stats.attackPower * 2) +
                                     (generated.stats.armor * 2) + (primaryBonus * 3));

  addItem(generated);
  return generated.id;
}
