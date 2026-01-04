#include <functional>

class Coordinate {
public:
  Coordinate() = delete;
  Coordinate(int x, int y) : x(x), y(y) {}

  bool operator==(const Coordinate& rhs) const { return x == rhs.x && y == rhs.y; }

public:
  int x;
  int y;
};

template <> struct std::hash<Coordinate> {
  std::size_t operator()(const Coordinate& coordinate) const {
    return std::hash<int>{}(coordinate.x) ^ (std::hash<int>{}(coordinate.y) << 1);
  }
};