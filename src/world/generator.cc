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
  regions.emplace_back(RegionType::SpawnRegion, 12, 12, 18, 18);
  regions.emplace_back(RegionType::SpawnRegion, 90, 20, 22, 18);
  regions.emplace_back(RegionType::SpawnRegion, 24, 88, 20, 24);
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
