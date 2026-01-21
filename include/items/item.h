#pragma once

#include <string>
#include <unordered_set>

enum class ItemSlot { Weapon = 0, Shield, Shoulders, Chest, Pants, Boots, Cape };

enum class CharacterClass { Any = 0, Warrior, Mage, Archer, Rogue };

enum class WeaponType { None = 0, OneHandedSword, TwoHandedSword, Polearm, Spear, Bow, Wand };

struct ItemStats {
  int attackPower = 0;
  int armor = 0;
};

struct ItemDef {
  int id = 0;
  std::string name;
  ItemSlot slot = ItemSlot::Weapon;
  int requiredLevel = 1;
  std::unordered_set<CharacterClass> allowedClasses;
  WeaponType weaponType = WeaponType::None;
  ItemStats stats;
};

struct ItemInstance {
  int itemId = 0;
};
