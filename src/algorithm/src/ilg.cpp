#include "ilg.h"

double InsertionLossGraph::getComponentLossCached(const std::shared_ptr<Cell>& cell)
{
  auto it = cell_loss_cache_.find(cell);
  if (it != cell_loss_cache_.end()) {
    return it->second;
  }
  std::string cell_type = cell->getType();
  double      loss      = 0.0;
  if (rc_.loss_comp.count(cell_type)) {
    loss = rc_.loss_comp.at(cell_type);
  } else {
    for (const auto& [comp_type, val] : rc_.loss_comp) {
      if (cell_type.find(comp_type) != std::string::npos) {
        loss = val;
        break;
      }
    }
  }
  cell_loss_cache_[cell] = loss;
  return loss;
}

void InsertionLossGraph::computeAndUpdateLosses(
    const std::unordered_set<std::string>&                     target_nets,
    const std::unordered_map<std::string, std::vector<Point>>& current_paths,
    const RoutingGrid&                                         global_grid)
{
  for (const auto& net_name : target_nets) {
    auto path_it = current_paths.find(net_name);
    if (path_it == current_paths.end())
      continue;
    const auto& path = path_it->second;
    NetLossInfo info;
    if (path.size() < 2) {
      net_losses_[net_name] = info;
      updateEdgeWeight(net_name, 0.0);
      continue;
    }
    double actual_path_length = 0.0;
    int    bends              = 0;
    int    crossings          = 0;
    for (size_t i = 1; i < path.size(); ++i) {
      double dx1 = path[i].x() - path[i - 1].x();
      double dy1 = path[i].y() - path[i - 1].y();
      actual_path_length += std::hypot(dx1, dy1);
      if (i + 1 < path.size()) {
        double dx2 = path[i + 1].x() - path[i].x();
        double dy2 = path[i + 1].y() - path[i].y();
        if ((std::abs(dx1) > EPSILON && std::abs(dy2) > EPSILON)
            || (std::abs(dy1) > EPSILON && std::abs(dx2) > EPSILON)) {
          bends++;
        }
        GridIndex gidx = global_grid.toIndex(path[i]);
        auto      it   = global_grid.usage.find(gidx);
        if (it != global_grid.usage.end() && it->second > 1) {
          crossings++;
        }
      }
    }
    info.propagation_loss = actual_path_length * rc_.loss_pp;
    info.bend_loss        = bends * rc_.loss_bend;
    info.crossing_loss    = crossings * rc_.loss_cr;
    info.total_loss = info.propagation_loss + info.bend_loss + info.crossing_loss;
    net_losses_[net_name] = info;
    updateEdgeWeight(net_name, info.total_loss);
  }
}
void InsertionLossGraph::updateSinglePathLossCache(size_t path_idx)
{
  const auto& topo = path_topologies_[path_idx];
  double new_total = topo.fixed_comp_loss;
  for (const auto& net_name : topo.unique_net_names) {
    auto it = net_losses_.find(net_name);
    if (it != net_losses_.end()) {
      new_total += it->second.total_loss;
    }
  }
  path_loss_cache_[path_idx] = new_total;
}

InsertionLossGraph::InsertionLossGraph(const RouteConfig& rc) : rc_(rc) {}

void InsertionLossGraph::setDatapaths(const std::vector<std::vector<std::shared_ptr<Pin>>>& paths)
{
  datapaths_ = paths;
  path_loss_cache_.assign(paths.size(), 0.0);
  path_topologies_.clear();
  path_topologies_.resize(paths.size());
  net_to_paths_.clear();
  for (size_t i = 0; i < datapaths_.size(); ++i) {
    std::unordered_set<std::shared_ptr<Cell>> unique_cells;
    std::unordered_set<std::string>           unique_nets;
    for (const auto& pin : datapaths_[i]) {
      unique_cells.insert(pin->getHostCell());
      if (auto net = pin->getHostNet()) {
        std::string net_name = net->getName();
        net_to_paths_[net_name].push_back(i);
        unique_nets.insert(net_name);
      }
    }
    double comp_loss = 0.0;
    for (const auto& cell : unique_cells) {
      comp_loss += getComponentLossCached(cell);
    }
    path_topologies_[i].fixed_comp_loss = comp_loss;
    path_topologies_[i].unique_net_names.assign(unique_nets.begin(),
                                                unique_nets.end());
  }
  std::cout << "[IL Graph] Loaded " << datapaths_.size()
            << " datapaths and pre-computed static topologies.\n";
}
void InsertionLossGraph::addEdge(const std::string& net_name, const InsertionLossEdge& edge)
{
  edges_[net_name] = edge;
}
bool InsertionLossGraph::updateEdgeWeight(const std::string& net_name, double new_loss)
{
  auto it = edges_.find(net_name);
  if (it != edges_.end()) {
    it->second.edge_weight = new_loss;
    return true;
  }
  return false;
}
std::vector<DatapathLossResult> InsertionLossGraph::getTopKDatapaths(int k) const
{
  std::vector<DatapathLossResult> results;
  if (path_loss_cache_.empty()) return results;
  size_t actual_k = std::min(static_cast<size_t>(k), path_loss_cache_.size());
  
  std::vector<size_t> indices(path_loss_cache_.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::partial_sort(indices.begin(), indices.begin() + actual_k, indices.end(),
                    [this](size_t a, size_t b) {
                      return path_loss_cache_[a] > path_loss_cache_[b];
                    });
  for (size_t i = 0; i < actual_k; ++i) {
    size_t idx = indices[i];
    results.push_back({idx, path_loss_cache_[idx], 0.0, 0.0, datapaths_[idx]});
  }
  return results;
}
void InsertionLossGraph::buildInitialGraphLosses(
    const std::unordered_map<std::string, std::vector<Point>>& routed_paths,
    const RoutingGrid&                                         global_grid)
{
  std::cout << "[IL Graph] Building initial graph losses and spatial index...\n";
  std::unordered_set<std::string> all_nets;
  grid_to_nets_.clear();
  for (const auto& [net_name, path] : routed_paths) {
    all_nets.insert(net_name);
    for (size_t i = 1; i + 1 < path.size(); ++i) {
      grid_to_nets_[global_grid.toIndex(path[i])].push_back(net_name);
    }
  }
  computeAndUpdateLosses(all_nets, routed_paths, global_grid);
  
  for (size_t i = 0; i < datapaths_.size(); ++i) {
    updateSinglePathLossCache(i);
  }
}
void InsertionLossGraph::incrementalUpdate(
    const std::shared_ptr<Cell>& moved_cell,
    const std::unordered_map<std::string, std::vector<Point>>& old_affected_paths,
    const std::unordered_map<std::string, std::vector<Point>>& new_affected_paths,
    const std::unordered_map<std::string, std::vector<Point>>& global_paths,
    const RoutingGrid&                                         global_grid)
{
  std::unordered_set<std::string>         target_nets;
  std::unordered_set<std::string>         directly_affected;
  std::unordered_set<GridIndex, GridHash> affected_grids;
  for (const auto& [pin_name, pin] : moved_cell->getPins()) {
    if (auto net = pin->getHostNet()) {
      std::string net_name = net->getName();
      directly_affected.insert(net_name);
      target_nets.insert(net_name);
      auto collect_grids = [&](const std::unordered_map<std::string, std::vector<Point>>& path_map) {
        auto it = path_map.find(net_name);
        if (it != path_map.end()) {
          for (size_t i = 1; i + 1 < it->second.size(); ++i) {
            affected_grids.insert(global_grid.toIndex(it->second[i]));
          }
        }
      };
      collect_grids(old_affected_paths);
      collect_grids(new_affected_paths);
    }
  }
  for (const auto& gidx : affected_grids) {
    auto it = grid_to_nets_.find(gidx);
    if (it != grid_to_nets_.end()) {
      for (const auto& net_name : it->second) {
        target_nets.insert(net_name);
      }
    }
  }
  computeAndUpdateLosses(target_nets, global_paths, global_grid);
  std::unordered_set<size_t> affected_path_indices;
  for (const auto& net_name : target_nets) {
    auto it = net_to_paths_.find(net_name);
    if (it != net_to_paths_.end()) {
      for (size_t idx : it->second) affected_path_indices.insert(idx);
    }
  }
  for (size_t idx : affected_path_indices) {
    updateSinglePathLossCache(idx);
  }
  
  for (const auto& net_name : directly_affected) {
    auto it = global_paths.find(net_name);
    if (it != global_paths.end()) {
      for (size_t i = 1; i + 1 < it->second.size(); ++i) {
        grid_to_nets_[global_grid.toIndex(it->second[i])].push_back(net_name);
      }
    }
  }
}
void InsertionLossGraph::analyzeDatapathLosses(int top_k) const
{
  auto top_results = getTopKDatapaths(top_k);
  std::cout << "\n========== TOP-" << top_results.size() << " CRITICAL DATAPATHS ==========\n";
  for (size_t r = 0; r < top_results.size(); ++r) {
    const auto& res = top_results[r];
    const auto& topo = path_topologies_[res.path_index];
    
    double net_loss = 0.0;
    for (const auto& name : topo.unique_net_names) {
      if (net_losses_.count(name)) net_loss += net_losses_.at(name).total_loss;
    }
    std::cout << "Rank " << r + 1 << " [Index: " << res.path_index << "]\n"
              << "  -> Total IL: " << res.total_loss << " dB\n"
              << "  -> Breakdown : Comp = " << topo.fixed_comp_loss 
              << " dB, Net = " << net_loss << " dB\n";
  }
  std::cout << "======================================================\n\n";
}
std::vector<std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>> InsertionLossGraph::getCritCells() const
{
  std::vector<std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>> critical_list;
  auto top_results = getTopKDatapaths(1);
  if (top_results.empty()) return critical_list;
  for (const auto& pin : top_results[0].path_elements) {
    if (!pin) continue;
    auto cell = pin->getHostCell();
    if (!cell || cell->getName().compare(0, 2, "gc") == 0) continue;
    if (!critical_list.empty() && critical_list.back().first == cell) {
      critical_list.back().second.push_back(pin);
    } else {
      critical_list.push_back({cell, {pin}});
    }
  }
  return critical_list;
}
double InsertionLossGraph::getMaxIL() const
{
  if (path_loss_cache_.empty()) return 0.0;
  return *std::max_element(path_loss_cache_.begin(), path_loss_cache_.end());
}
