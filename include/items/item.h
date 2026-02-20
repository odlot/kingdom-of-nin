#pragma once

#include <string>
#include <unordered_set>

enum class ItemSlot { Weapon = 0, Shield, Shoulders, Chest, Pants, Boots, Cape };

enum class CharacterClass { Any = 0, Warrior, Mage, Archer, Rogue };

enum class WeaponType {
  None = 0,
  OneHandedSword,
  TwoHandedSword,
  Polearm,
  Spear,
  Bow,
  Wand,
  Dagger
};

enum class ItemRarity { Common = 0, Rare, Epic };

struct PrimaryStatBonuses {
  int strength = 0;
  int dexterity = 0;
  int intellect = 0;
  int luck = 0;
};

struct WeaponEquipmentStats {
  PrimaryStatBonuses primary;
};

struct ArmorEquipmentStats {
  int armor = 0;
  PrimaryStatBonuses primary;
};

struct ProjectileStats {
  float speed = 0.0f;
  float radius = 0.0f;
  float trailLength = 0.0f;
};

struct ItemDef {
  int id = 0;
  std::string name;
  ItemSlot slot = ItemSlot::Weapon;
  ItemRarity rarity = ItemRarity::Common;
  int requiredLevel = 1;
  std::unordered_set<CharacterClass> allowedClasses;
  WeaponType weaponType = WeaponType::None;
  WeaponEquipmentStats weaponStats;
  ArmorEquipmentStats armorStats;
  ProjectileStats projectile;
  int price = 0;
};

struct ItemInstance {
  int itemId = 0;
};

inline const char* itemRarityName(ItemRarity rarity) {
  switch (rarity) {
  case ItemRarity::Common:
    return "Common";
  case ItemRarity::Rare:
    return "Rare";
  case ItemRarity::Epic:
    return "Epic";
  }
  return "Unknown";
}

inline bool isWeaponSlot(ItemSlot slot) {
  return slot == ItemSlot::Weapon;
}

inline bool isWeaponItem(const ItemDef& def) {
  return isWeaponSlot(def.slot);
}

inline const PrimaryStatBonuses& primaryStatsForItem(const ItemDef& def) {
  return isWeaponItem(def) ? def.weaponStats.primary : def.armorStats.primary;
}

inline PrimaryStatBonuses& primaryStatsForItem(ItemDef& def) {
  return isWeaponItem(def) ? def.weaponStats.primary : def.armorStats.primary;
}

inline int armorForItem(const ItemDef& def) {
  return isWeaponItem(def) ? 0 : def.armorStats.armor;
}

inline bool canEquipForClass(const ItemDef& def, CharacterClass characterClass) {
  if (def.allowedClasses.empty()) {
    return true;
  }
  if (def.allowedClasses.count(CharacterClass::Any) > 0) {
    return true;
  }
  return def.allowedClasses.count(characterClass) > 0;
}

inline bool meetsEquipRequirements(const ItemDef& def, int level, CharacterClass characterClass) {
  if (level < def.requiredLevel) {
    return false;
  }
  return canEquipForClass(def, characterClass);
}
