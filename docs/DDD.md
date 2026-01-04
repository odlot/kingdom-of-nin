# Detailed Design Document (DDD)

## System Architecture

- C++ core game loop with SDL rendering and input.
- ECS-driven gameplay systems (combat, AI, loot, world events).
- Content pipeline that loads assets and data at startup; hot-reload optional.

## Technology Stack

- C++20, CMake build system.
- SDL3 and SDL_ttf for windowing, input, and text.
- spdlog for logging.

## Data Models

- `Entity`: id, components, tags.
- `Character`: stats, class, abilities, inventory.
- `Item`: rarity, affixes, slot, level.
- `World`: seed, biome graph, dungeon layout.
- `Quest/Event`: triggers, objectives, rewards, timers.

## Item & Stat Prototype Plan

- Data-driven item definitions separate from runtime instances.
- `ItemDef`: id, name, slot, required level, class restrictions, stat modifiers.
- `ItemInstance`: reference to `ItemDef` (future: affixes/durability).
- `InventoryComponent`: list of item instances.
- `EquipmentComponent`: slot -> equipped item instance.
- `StatsComponent`: base stats (attack power, armor; extend later).
- Computed stats built from base + equipped item modifiers.

## API Design

- Internal engine APIs for systems: `Update(dt)`, `Render()`, `HandleInput()`.
- Data-driven registries for classes, items, and enemies.
- Event bus for cross-system communication (combat, loot, UI).

## Scalability Considerations

- Deterministic simulation for future multiplayer.
- Clear separation between simulation and rendering.
- Asset streaming and chunked world loading as content grows.
