#pragma once

enum class RegionType { StartingZone = 0, DungeonEntrance, SpawnRegion };

struct Region {
  Region(RegionType type, int x, int y, int width, int height)
      : type(type), x(x), y(y), width(width), height(height) {}

  bool contains(int px, int py) const {
    return px >= x && px < (x + width) && py >= y && py < (y + height);
  }

  RegionType type;
  int x;
  int y;
  int width;
  int height;
};
