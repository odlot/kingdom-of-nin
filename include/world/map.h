#pragma once

#include <unordered_map>
#include <vector>

#include "world/coordinate.h"
#include "world/region.h"
#include "world/tile.h"

class Map {
public:
  Map() = delete;
  Map(int width, int height, std::unordered_map<Coordinate, Tile> tiles,
      std::vector<Region> regions, Coordinate startingPosition);
  ~Map() = default;

  void print();
  const std::vector<Region>& getRegions() const { return regions; }
  const Coordinate& getStartingPosition() const { return startingPosition; }
  Tile getTile(int x, int y) const;
  bool isWalkable(int x, int y) const;
  bool isInside(int x, int y) const;
  int getWidth() const { return width; }
  int getHeight() const { return height; }

private:
  std::unordered_map<Coordinate, Tile> tiles;
  std::vector<Region> regions;
  Coordinate startingPosition;
  int width;
  int height;
};

inline bool Map::isInside(int x, int y) const {
  return x >= 0 && x < width && y >= 0 && y < height;
}

inline bool Map::isWalkable(int x, int y) const {
  if (!isInside(x, y)) {
    return false;
  }
  auto it = tiles.find(Coordinate(x, y));
  if (it == tiles.end()) {
    return false;
  }
  switch (it->second) {
  case Tile::Grass:
  case Tile::Town:
  case Tile::DungeonEntrance:
    return true;
  case Tile::Water:
  case Tile::Mountain:
    return false;
  }
  return false;
}

inline Tile Map::getTile(int x, int y) const {
  auto it = tiles.find(Coordinate(x, y));
  if (it == tiles.end()) {
    return Tile::Grass;
  }
  return it->second;
}
