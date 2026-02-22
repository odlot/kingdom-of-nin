# To-Do's

## Current Priority: Quality of Life & Miscellaneous

- Expand loot quality-of-life:
  - Loot filter modes (`upgrades only`, `hide common`, `class-only`)
  - Auto-loot options for low-value/common drops
  - Ground loot cap and oldest-first cleanup safety limits
- Improve inventory and item UX:
  - Sort/stack actions
  - Mark as junk + sell-junk flow
  - Better compare tooltips (secondary stats and requirements clarity)
- Improve combat readability:
  - Damage number density controls
  - Better buff/debuff visibility and icon clarity
  - Colorblind-safe telegraph palettes
- Improve navigation and onboarding:
  - Minimap markers for important points of interest
  - Better region labels and pathing hints for key objectives
  - Better early-game guidance and interaction prompts
- Improve controls/settings UX:
  - Keybind remapping
  - Persistent settings profiles
  - Accessibility toggles (motion/intensity/text scale)

## Phase 1: Core Loop Foundation

- Entity component system
  - Collision
  - Health, Mana
  - Level, Experience
- A base map with a starting zone from which one can access dungeons that has procedurally generated maps
- Regions (or areas) where mobs spawn
- HUD

## Phase 2: Combat and Progression Basics

- Warrior, Mage, Archer, Rogue class (start with one to validate combat feel)
- Skill system
- Stat system
- Move attack tuning (range, arc, cooldown, projectile feel) from weapon type defaults to per-weapon attributes
- Start as Adventurer and unlock class choice at level 10
- Primary statistics like intellect (INT), strength (STR), luck (LUK), and dexterity (DEX)
  - Things like INT influence Mana, STR influences HP, Luck influences Critical Chance / Damage, DEX influences Accuracy and Dodge / Evasion
  - Attribute component?
- "Secondary" statistics like Critical Damage, Critical Chance, Parry, Dodge / Evasion, Accuracy ...
- Level system

## Phase 3: Loot and Gear

- Inventory system
  - Items
- Equipment system

## Phase 4: Events and Variety

- Event system

## Dungeon Features (Queued Next)

- Dungeon run lifecycle (`start`, `floor clear`, `fail`, `complete`, `reward`, `extract`)
- Affix framework (registry + weighted roll + deterministic seed per run)
- First affix pack (`Fortified`, `Frenzied`, `Volcanic`, `Arcane Surge`, `Glass Cannon`)
- Affix conflict/stacking rules (prevent broken combinations)
- Dungeon reward scaling (XP/loot multiplier based on affix difficulty score)
- Run HUD panel (active affixes, floor, timer, score multiplier)
- End-of-run summary screen (affixes, floors, deaths, rewards, time)
- Test dungeon integration for affix debugging (quick-start run + repeat button)
- Elite/boss dungeon encounters with telegraphed mechanics
- Affix simulation tests (distribution and pacing sanity checks)
