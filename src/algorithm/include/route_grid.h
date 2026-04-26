#pragma once
struct GridIndex
{
  int  x, y;
  bool operator==(const GridIndex& o) const { return x == o.x && y == o.y; }
  bool operator!=(const GridIndex& o) const { return x != o.x || y != o.y; }
  bool operator<(const GridIndex& o) const { return x < o.x || y < o.y; }
};

struct GridHash
{
  size_t operator()(const GridIndex& idx) const
  {
    return std::hash<int>()(idx.x) ^ (std::hash<int>()(idx.y) << 1);
  }
};

enum class Direction
{
  NONE,
  NORTH,
  SOUTH,
  EAST,
  WEST,
  HORIZONTAL,
  VERTICAL
};

class RoutingGrid
{
 public:
  int                                          max_capacity;
  double                                       step;
  Point                                        origin;
  std::unordered_map<GridIndex, int, GridHash> usage;

  RoutingGrid() : step(20.0), origin(Point(0, 0)), max_capacity(4) {}
  RoutingGrid(double s, Point o, int cap)
      : step(s), origin(o), max_capacity(cap){}

  GridIndex toIndex(const Point& p) const
  {
    return {static_cast<int>(
                std::round((bg::get<0>(p) - bg::get<0>(origin)) / step)),
            static_cast<int>(
                std::round((bg::get<1>(p) - bg::get<1>(origin)) / step))};
  }

  Point toPoint(const GridIndex& idx) const
  {
    return Point(bg::get<0>(origin) + idx.x * step,
                 bg::get<1>(origin) + idx.y * step);
  }

  double getCost(const GridIndex& idx) const
  {
    auto   it              = usage.find(idx);
    int    current_usage   = (it != usage.end()) ? it->second : 0;
    double occupancy_ratio = static_cast<double>(current_usage) / max_capacity;
    double congestion_cost = std::pow(occupancy_ratio, 2) * 10.0;
    if (current_usage >= max_capacity)
      congestion_cost += 100.0;
    return 1.0 + congestion_cost;
  }

  bool isFull(const GridIndex& idx) const
  {
    auto it = usage.find(idx);
    return it != usage.end() && it->second >= max_capacity;
  }

  void clear() { usage.clear(); }
};
