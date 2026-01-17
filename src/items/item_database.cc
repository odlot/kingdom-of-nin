#include "items/item_database.h"

ItemDatabase::ItemDatabase() {
  ItemDef basicSword;
  basicSword.id = 1;
  basicSword.name = "Basic Sword";
  basicSword.slot = ItemSlot::Weapon;
  basicSword.requiredLevel = 1;
  basicSword.allowedClasses = {CharacterClass::Any};
  basicSword.weaponType = WeaponType::OneHandedSword;
  basicSword.stats.attackPower = 5;
  addItem(basicSword);

  ItemDef basicSpear;
  basicSpear.id = 5;
  basicSpear.name = "Basic Spear";
  basicSpear.slot = ItemSlot::Weapon;
  basicSpear.requiredLevel = 1;
  basicSpear.allowedClasses = {CharacterClass::Any};
  basicSpear.weaponType = WeaponType::Spear;
  basicSpear.stats.attackPower = 6;
  addItem(basicSpear);

  ItemDef basicShield;
  basicShield.id = 2;
  basicShield.name = "Basic Shield";
  basicShield.slot = ItemSlot::Shield;
  basicShield.requiredLevel = 1;
  basicShield.allowedClasses = {CharacterClass::Any};
  basicShield.stats.armor = 3;
  addItem(basicShield);

  ItemDef basicBoots;
  basicBoots.id = 3;
  basicBoots.name = "Basic Boots";
  basicBoots.slot = ItemSlot::Boots;
  basicBoots.requiredLevel = 1;
  basicBoots.allowedClasses = {CharacterClass::Any};
  basicBoots.stats.armor = 1;
  addItem(basicBoots);

  ItemDef basicChest;
  basicChest.id = 4;
  basicChest.name = "Basic Tunic";
  basicChest.slot = ItemSlot::Chest;
  basicChest.requiredLevel = 1;
  basicChest.allowedClasses = {CharacterClass::Any};
  basicChest.stats.armor = 2;
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
