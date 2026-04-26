#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "design.h"
#include "route_config.h"
#include "route_grid.h"


struct NetLossInfo
{
  double propagation_loss = 0.0;
  double bend_loss        = 0.0;
  double crossing_loss    = 0.0;
  double total_loss       = 0.0;
};

struct InsertionLossEdge
{
  std::shared_ptr<Cell> src_cell;
  std::shared_ptr<Cell> dst_cell;
  std::shared_ptr<Net>  net;
  double                edge_weight = 0.0;
};

struct DatapathLossResult
{
  size_t                            path_index;
  double                            total_loss;
  double                            comp_loss;
  double                            net_loss;
  std::vector<std::shared_ptr<Pin>> path_elements;
};

struct PathTopology
{
  double                   fixed_comp_loss = 0.0;
  std::vector<std::string> unique_net_names;
};

using Point        = boost::geometry::model::d2::point_xy<double>;
using Polygon      = boost::geometry::model::polygon<Point>;
using MultiPolygon = boost::geometry::model::multi_polygon<Polygon>;

struct Patch
{
  Polygon poly;
  int     w;
};

struct RegionWithWeights
{
  Polygon          poly;
  std::vector<int> weights;
};

using CellPtr = std::shared_ptr<Cell>;


class InsertionLossGraph
{
 private:
  std::unordered_map<std::string, InsertionLossEdge> edges_;
  std::unordered_map<std::string, NetLossInfo>       net_losses_;
  RouteConfig                                        rc_;
  static constexpr double                            EPSILON = 1e-3;

  std::vector<std::vector<std::shared_ptr<Pin>>> datapaths_;
  std::vector<PathTopology>                      path_topologies_;

  std::vector<double>                                  path_loss_cache_;
  std::unordered_map<std::string, std::vector<size_t>> net_to_paths_;
  std::unordered_map<std::shared_ptr<Cell>, double>    cell_loss_cache_;

  std::unordered_map<GridIndex, std::vector<std::string>, GridHash> grid_to_nets_;

  double getComponentLossCached(const std::shared_ptr<Cell>& cell);

  void computeAndUpdateLosses(
      const std::unordered_set<std::string>&                     target_nets,
      const std::unordered_map<std::string, std::vector<Point>>& current_paths,
      const RoutingGrid&                                         global_grid);

  void updateSinglePathLossCache(size_t path_idx);

 public:
  InsertionLossGraph(const RouteConfig& rc);

  void setDatapaths(const std::vector<std::vector<std::shared_ptr<Pin>>>& paths);

  void addEdge(const std::string& net_name, const InsertionLossEdge& edge);

  bool updateEdgeWeight(const std::string& net_name, double new_loss);

  std::vector<DatapathLossResult> getTopKDatapaths(int k) const;

  void buildInitialGraphLosses(
      const std::unordered_map<std::string, std::vector<Point>>& routed_paths,
      const RoutingGrid&                                         global_grid);

  void incrementalUpdate(
      const std::shared_ptr<Cell>& moved_cell,
      const std::unordered_map<std::string, std::vector<Point>>& old_affected_paths,
      const std::unordered_map<std::string, std::vector<Point>>& new_affected_paths,
      const std::unordered_map<std::string, std::vector<Point>>& global_paths,
      const RoutingGrid&                                         global_grid);

  void analyzeDatapathLosses(int top_k = 5) const;

  std::vector<std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>> getCritCells() const;

  double getMaxIL() const;
};