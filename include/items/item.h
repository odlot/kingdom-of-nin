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

struct ItemStats {
  int attackPower = 0;
  int armor = 0;
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
  int requiredLevel = 1;
  std::unordered_set<CharacterClass> allowedClasses;
  WeaponType weaponType = WeaponType::None;
  ItemStats stats;
  ProjectileStats projectile;
  int price = 0;
};

struct ItemInstance {
  int itemId = 0;
};

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
