#include "world/generator.h"
#include "world/map.h"
#include "world/tile.h"

std::unique_ptr<Map> Generator::generate() {
  int width = 128;
  int height = 128;
  std::unordered_map<Coordinate, Tile> tiles;
  std::vector<Region> regions;
  int startZoneSize = 20;
  int startZoneX = (width - startZoneSize) / 2;
  int startZoneY = (height - startZoneSize) / 2;
  Coordinate startingPosition(startZoneX + startZoneSize / 2, startZoneY + startZoneSize / 2);
  regions.emplace_back(RegionType::StartingZone, startZoneX, startZoneY, startZoneSize,
                       startZoneSize);
  regions.emplace_back(RegionType::SpawnRegion, 12, 12, 16, 16, 1, 5, 0);
  regions.emplace_back(RegionType::SpawnRegion, 34, 10, 16, 16, 6, 10, 1);
  regions.emplace_back(RegionType::SpawnRegion, 56, 8, 16, 16, 11, 15, 2);
  regions.emplace_back(RegionType::SpawnRegion, 84, 12, 18, 16, 16, 20, 3);
  regions.emplace_back(RegionType::SpawnRegion, 102, 28, 18, 18, 21, 25, 4);
  regions.emplace_back(RegionType::SpawnRegion, 102, 54, 18, 18, 26, 30, 5);
  regions.emplace_back(RegionType::SpawnRegion, 96, 82, 20, 18, 31, 35, 6);
  regions.emplace_back(RegionType::SpawnRegion, 74, 98, 18, 18, 36, 40, 7);
  regions.emplace_back(RegionType::SpawnRegion, 48, 102, 18, 18, 41, 45, 8);
  regions.emplace_back(RegionType::SpawnRegion, 24, 96, 18, 20, 46, 50, 9);
  regions.emplace_back(RegionType::SpawnRegion, 10, 72, 18, 18, 51, 55, 10);
  regions.emplace_back(RegionType::SpawnRegion, 8, 44, 18, 18, 56, 60, 11);
  regions.emplace_back(RegionType::GoblinCamp, 72, 72, 18, 18);
  regions.emplace_back(RegionType::DungeonEntrance, 20, 64, 1, 1);
  regions.emplace_back(RegionType::DungeonEntrance, 64, 20, 1, 1);
  regions.emplace_back(RegionType::DungeonEntrance, 108, 96, 1, 1);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (x >= startZoneX && x < startZoneX + startZoneSize && y >= startZoneY &&
          y < startZoneY + startZoneSize) {
        tiles[Coordinate(x, y)] = Tile::Town;
      } else if (x % 5 == 0 && y % 5 == 0) {
        tiles[Coordinate(x, y)] = Tile::Water;
      } else if (x % 7 == 0 && y % 7 == 0) {
        tiles[Coordinate(x, y)] = Tile::Mountain;
      } else {
        tiles[Coordinate(x, y)] = Tile::Grass;
      }
    }
  }
  for (const auto& region : regions) {
    if (region.type == RegionType::DungeonEntrance) {
      tiles[Coordinate(region.x, region.y)] = Tile::DungeonEntrance;
    }
  }
  return std::make_unique<Map>(width, height, tiles, std::move(regions), startingPosition);
}
