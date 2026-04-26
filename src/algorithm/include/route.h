#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Database;
class Design;
class Library;
class Net;
struct RouteConfig;

#define MY_ASSERT(condition, message)                             \
  do {                                                            \
    if (!(condition)) {                                           \
      std::cerr << "Failed: " << #condition << "\n"               \
                << "Info: " << message << "\n"                    \
                << "File: " << __FILE__ << ", Line: " << __LINE__ \
                << std::endl;                                     \
      std::abort();                                               \
    }                                                             \
  } while (0)

class RouteManager
{
 public:
  struct Setting
  {
    int width;
    int spacing;
    int pitch;
  };
  RouteManager(std::shared_ptr<Database> _database, const RouteConfig& _config);
  void routeAll();
  void writeBack();

 private:
  // double routeAngle(std::pair<int, int>& p1, std::pair<int, int>& p2)
  std::vector<std::pair<double, double>> postProcessing(
      const std::string&                      path_name,
      std::vector<std::pair<int, int>>&       path_data,
      std::vector<int>&                       angles,
      std::vector<std::pair<double, double>>& pts);
  double                    routeAngle(std::pair<double, double>& p1,
                                       std::pair<double, double>& p2);
  std::pair<double, double> ll;
  std::vector<std::vector<std::vector<int>>>                        map;
  std::unordered_map<std::string, std::vector<std::pair<int, int>>> paths;
  Setting                                                           setting;
  std::shared_ptr<Database>                                         _database;
  std::shared_ptr<Design>                                           _design;
  std::shared_ptr<Library>                                          _library;
};

class Router
{
 public:
  struct GridPoint
  {
    enum DIRECTION
    {
      UNSUPPORT = -1,
      RIGHT,
      UP,
      LEFT,
      DOWN
    };
    int                        x    = 0;
    int                        y    = 0;
    int                        cost = 0;
    int                        len  = 0;
    DIRECTION                  dire = DIRECTION::UNSUPPORT;
    std::shared_ptr<GridPoint> prev;

    // GridPoint(int p1, int p2):
    //     x(p1), y(p2), cost(0), prev(nullptr){};
    GridPoint(int                        p1,
              int                        p2,
              int                        l = 0,
              DIRECTION                  d = UNSUPPORT,
              int                        c = 0,
              std::shared_ptr<GridPoint> p = nullptr)
        : x(p1), y(p2), len(l), dire(d), cost(c), prev(p){};
    void calcDirection(std::shared_ptr<GridPoint>& other)
    {
      if (x < other->x)
        dire = DIRECTION::LEFT;
      else if (x > other->x)
        dire = DIRECTION::RIGHT;
      if (y < other->y)
        dire = DIRECTION::DOWN;
      else if (y > other->y)
        dire = DIRECTION::UP;
    }
  };
  struct CmpCost
  {
    bool operator()(const std::shared_ptr<GridPoint>& lhs,
                    const std::shared_ptr<GridPoint>& rhs) const
    {
      return lhs->cost > rhs->cost;
    }
  };
  GridPoint::DIRECTION calcDirection(std::shared_ptr<GridPoint>& other,
                                     int                         x,
                                     int                         y)
  {
    if (x < other->x)
      return GridPoint::DIRECTION::LEFT;
    else if (x > other->x)
      return GridPoint::DIRECTION::RIGHT;
    if (y < other->y)
      return GridPoint::DIRECTION::DOWN;
    else if (y > other->y)
      return GridPoint::DIRECTION::UP;
    return GridPoint::DIRECTION::UNSUPPORT;
  }
  Router(std::vector<std::vector<std::vector<int>>>& m,
         const std::pair<double, double>&            ll,
         std::shared_ptr<Net>&                       n,
         const RouteManager::Setting&                set,
         int                                         net_idx);
  std::vector<std::pair<int, int>> operator()(int i, std::string net_name);

 private:
  int  net_idx = 0;
  bool checkBoundary(const GridPoint& p);
  bool checkBoundary(const std::shared_ptr<GridPoint>& p);
  void setForbiddenAccess(const GridPoint& p, int access_direction);
  std::pair<double, double>     ll;
  std::set<std::pair<int, int>> target;
  std::priority_queue<std::shared_ptr<GridPoint>,
                      std::vector<std::shared_ptr<GridPoint>>,
                      CmpCost>
                                              pq;
  const std::shared_ptr<Net>&                 net;
  const RouteManager::Setting&                setting;
  std::vector<std::vector<std::vector<int>>>& map;
  std::vector<std::pair<int, int>>            path;
  std::vector<GridPoint>                      endpoints;
  std::vector<std::vector<int>>               cost;
  std::vector<std::vector<int>>               weight;
};