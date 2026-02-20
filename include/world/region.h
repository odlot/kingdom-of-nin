#pragma once

enum class RegionType { StartingZone = 0, DungeonEntrance, SpawnRegion, GoblinCamp };

struct Region {
  Region(RegionType type, int x, int y, int width, int height, int minLevel = 1, int maxLevel = 1,
         int spawnTier = 0)
      : type(type), x(x), y(y), width(width), height(height), minLevel(minLevel),
        maxLevel(maxLevel), spawnTier(spawnTier) {}

  bool contains(int px, int py) const {
    return px >= x && px < (x + width) && py >= y && py < (y + height);
  }

  RegionType type;
  int x;
  int y;
  int width;
  int height;
  int minLevel;
  int maxLevel;
  int spawnTier;
};
