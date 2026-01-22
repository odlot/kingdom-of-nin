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
