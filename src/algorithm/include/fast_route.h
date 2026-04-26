#pragma once

#include <algorithm>
#include <boost/geometry/algorithms/closest_points.hpp>
#include <boost/geometry/geometries/multi_point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <iomanip>


#include "design.h"
#include "route_config.h"

#include "route_grid.h"
#include "ilg.h"

enum class PinDirection
{
  UNKNOWN,
  INPUT,
  OUTPUT
};



inline Direction degreeToDirection(double angle);

inline Direction getAxis(Direction dir);

inline Direction reverseDir(Direction dir);

inline bool isPointEqual(const Point& a, const Point& b, double epsilon = 1e-5)
{
  return std::abs(bg::get<0>(a) - bg::get<0>(b)) < epsilon
         && std::abs(bg::get<1>(a) - bg::get<1>(b)) < epsilon;
}


struct PinEscapeInfo
{
  GridIndex coarse_gcell_idx;
  Point     physical_pos;
  Direction escape_dir;
  bool operator==(const PinEscapeInfo& other) const
  {
    return coarse_gcell_idx == other.coarse_gcell_idx
           && isPointEqual(physical_pos, other.physical_pos)
           && escape_dir == other.escape_dir;
  }
};

struct RoutingInfo
{
  RoutingGrid                                             grid_;
  std::unordered_map<std::string, std::vector<Point>>     all_paths_;
  std::unordered_map<std::shared_ptr<Pin>, PinEscapeInfo> escape_map_;
  std::shared_ptr<InsertionLossGraph>                     ilg_;
};


class GlobalRouter
{
 private:
  std::shared_ptr<Design>                                 design_;
  RoutingGrid                                             grid_;
  std::unordered_map<std::string, std::vector<Point>>     all_paths_;
  std::unordered_map<std::shared_ptr<Pin>, PinEscapeInfo> escape_map_;

  std::shared_ptr<InsertionLossGraph> ilg_;

  double lx_, ly_, ux_, uy_;

  void addCellsToGrid();


  void precalculatePinEscapes();


  std::vector<Point> fineGrainedRoute(const std::vector<GridIndex>& coarse_path,
                                      const PinEscapeInfo& start_escape,
                                      const PinEscapeInfo& end_escape) const;

  bool routeSingleNet(const std::string& net_name, std::shared_ptr<Net> net);
  
 public:
  GlobalRouter(std::shared_ptr<Design> design,
               double                  coarse_step = 20.0,
               int                     capacity = 4);

  bool checkEscapeMapCollisions() const;

  void setInsertionLossGraph(std::shared_ptr<InsertionLossGraph> ilg);
  const std::unordered_map<std::string, std::vector<Point>>& getAllPaths() const;
  const RoutingGrid& getRouterGrid() const;

  void initialRoute();

  bool rerouteCellNets(std::shared_ptr<Cell> moved_cell);


  RoutingInfo getRoutingInfo() const;

  void setRoutingInfo(const RoutingInfo& info);

  void loadRoutingInfo(const RoutingInfo& info);
  void exportToSVG(
      const std::string& filename,
      const std::vector<
          std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>>&
          critical_cells
      = {},
      const Polygon&                                           poly       = {},
      const std::vector<std::pair<Polygon, std::vector<int>>>& subRegions = {},
      const std::vector<Point>&                                X = {}) const;

  std::unordered_set<GridIndex, GridHash> findFreeGrids(
      const std::shared_ptr<Cell>& cell,
      double                       physical_margin) const;
 
  Polygon gridsToPolygon(
      const std::unordered_set<GridIndex, GridHash>& grids) const;

  Polygon getTargetRegion(std::shared_ptr<Cell> cell,
                          double                margin) const;

  std::vector<Polygon> toPolygonList(const MultiPolygon& multi) const;

  std::vector<Patch> createPatches(const Polygon& targetRegion,
                                   const Point&   ref,
                                   double         angle_deg,
                                   const Point&   pin_offset,
                                   double         r) const;

  std::vector<std::pair<Polygon, std::vector<int>>> getSubRegions(
      std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>
              cell_pair,
      Polygon targetRegion,
      double  radius = 5.0) const;

  // Sort subregions by weights
  void sortSubRegions(
      std::vector<std::pair<Polygon, std::vector<int>>>& subRegions);

  Point getMedian(std::pair<std::shared_ptr<Cell>,
                            std::vector<std::shared_ptr<Pin>>> cell_pair) const;

  bool isInRegion(const Point& p, const Polygon& poly) const;
  Point projectToRegion(const Point& p, const Polygon& poly) const;

  bool isBlocked(const std::shared_ptr<Cell>& cell) const;


  void incrementalPlace(double region_margin = 1000.0,
                        double bend_radius   = 5.0,
                        int    K             = 10);

  void analyzeDatapathLosses(int K);
};

