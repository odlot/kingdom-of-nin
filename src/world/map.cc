#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include <iostream>
#include <unordered_map>

#include "world/map.h"

Map::Map(int width, int height, std::unordered_map<Coordinate, Tile> tiles,
         std::vector<Region> regions, Coordinate startingPosition)
    : width(width), height(height), tiles(std::move(tiles)), regions(std::move(regions)),
      startingPosition(startingPosition) {}

void Map::print() {
  /*
  for (const auto &coordinate : tiles) {
    std::cout << "Tile at (" << coordinate.first.x << ", " << coordinate.first.y
              << "): ";
    switch (coordinate.second) {
    case Tile::Grass:
      std::cout << "Grass";
      break;
    case Tile::Water:
      std::cout << "Water";
      break;
    case Tile::Mountain:
      std::cout << "Mountain";
      break;
    }
    std::cout << std::endl;
  }
  */

  cv::Scalar grassColor(34, 139, 34);
  cv::Scalar waterColor(255, 0, 0);
  cv::Scalar mountainColor(128, 128, 128);
  cv::Scalar townColor(200, 180, 50);
  cv::Scalar dungeonColor(200, 90, 40);
  cv::Scalar startZoneOverlay(255, 240, 120);
  cv::Scalar spawnOverlay(80, 140, 220);
  cv::Scalar goblinOverlay(80, 200, 120);
  cv::Scalar dungeonOverlay(255, 120, 60);
  cv::Mat mapImage(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
  for (const auto& tile : tiles) {
    cv::Scalar color;
    switch (tile.second) {
    case Tile::Grass:
      color = grassColor;
      break;
    case Tile::Water:
      color = waterColor;
      break;
    case Tile::Mountain:
      color = mountainColor;
      break;
    case Tile::Town:
      color = townColor;
      break;
    case Tile::DungeonEntrance:
      color = dungeonColor;
      break;
    }
    for (const auto& region : regions) {
      if (region.contains(tile.first.x, tile.first.y)) {
        switch (region.type) {
        case RegionType::StartingZone:
          color = startZoneOverlay;
          break;
        case RegionType::SpawnRegion:
          color = spawnOverlay;
          break;
        case RegionType::GoblinCamp:
          color = goblinOverlay;
          break;
        case RegionType::DungeonEntrance:
          color = dungeonOverlay;
          break;
        }
      }
    }
    mapImage.at<cv::Vec3b>(tile.first.y, tile.first.x) = cv::Vec3b(color[0], color[1], color[2]);
  }
  cv::imwrite("map_debug.png", mapImage);
}
