#include "fast_route.h"
std::unordered_set<GridIndex, GridHash> GlobalRouter::findFreeGrids(
    const std::shared_ptr<Cell>& cell,
    double                       physical_margin = 100.0) const
{
  std::unordered_set<GridIndex, GridHash> free_grids;
  if (!cell || !design_)
    return free_grids;
  std::unordered_set<std::string> affected_net_names;
  for (auto& [net_name, net] : design_->getNets()) {
    for (auto pin : net->getPins()) {
      if (pin->getHostCell() == cell) {
        affected_net_names.insert(net_name);
        break;
      }
    }
  }
  std::unordered_map<GridIndex, int, GridHash> affected_path_usage;
  for (const auto& net_name : affected_net_names) {
    auto it = all_paths_.find(net_name);
    if (it != all_paths_.end()) {
      std::unordered_set<GridIndex, GridHash> visited_in_path;
      for (const auto& pt : it->second) {
        GridIndex idx = grid_.toIndex(pt);
        if (visited_in_path.insert(idx).second) {
          affected_path_usage[idx]++;
        }
      }
    }
  }
  auto   dim      = cell->getBoundingBox();
  auto   pos      = cell->getPosition();
  double cx       = pos.x();
  double cy       = pos.y();
  double origin_x = grid_.origin.get<0>();
  double origin_y = grid_.origin.get<1>();
  double step     = grid_.step;
  int c_min_x = static_cast<int>(
      std::floor((cx - dim.first / 2.0 - origin_x) / step));
  int c_min_y = static_cast<int>(
      std::floor((cy - dim.second / 2.0 - origin_y) / step));
  int c_max_x
      = static_cast<int>(std::ceil((cx + dim.first / 2.0 - origin_x) / step));
  int c_max_y = static_cast<int>(
      std::ceil((cy + dim.second / 2.0 - origin_y) / step));
  GridIndex seed_idx = grid_.toIndex(pos);
  int margin_cnt = static_cast<int>(std::ceil(physical_margin / step));
  int limit_offset_x = ((c_max_x - c_min_x) / 2) + margin_cnt;
  int limit_offset_y = ((c_max_y - c_min_y) / 2) + margin_cnt;
  auto isInsideCell = [&](const GridIndex& idx) {
    return idx.x >= c_min_x && idx.x <= c_max_x && idx.y >= c_min_y
           && idx.y <= c_max_y;
  };
  auto isFree = [&](const GridIndex& idx) {
    if (isInsideCell(idx))
      return true;
    auto it = grid_.usage.find(idx);
    if (it == grid_.usage.end())
      return true;
    int  current_usage = it->second;
    auto aff_it        = affected_path_usage.find(idx);
    if (aff_it != affected_path_usage.end()) {
      current_usage -= aff_it->second;
    }
    return current_usage <= 0;
  };
  std::queue<GridIndex>                   q;
  std::unordered_set<GridIndex, GridHash> visited;
  q.push(seed_idx);
  visited.insert(seed_idx);
  if (isFree(seed_idx))
    free_grids.insert(seed_idx);
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};
  while (!q.empty()) {
    GridIndex cur = q.front();
    q.pop();
    for (int d = 0; d < 4; ++d) {
      GridIndex nxt{cur.x + dx[d], cur.y + dy[d]};
      if (std::abs(nxt.x - seed_idx.x) > limit_offset_x
          || std::abs(nxt.y - seed_idx.y) > limit_offset_y)
        continue;
      if (visited.insert(nxt).second) {
        if (isFree(nxt)) {
          q.push(nxt);
          free_grids.insert(nxt);
        }
      }
    }
  }
  return free_grids;
}
Polygon GlobalRouter::gridsToPolygon(
    const std::unordered_set<GridIndex, GridHash>& grids) const
{
  if (grids.empty())
    throw std::runtime_error("Empty grids.");
  struct Edge
  {
    GridIndex p1, p2;
    Edge(GridIndex a, GridIndex b)
    {
      if (a.x < b.x || (a.x == b.x && a.y < b.y)) {
        p1 = a;
        p2 = b;
      } else {
        p1 = b;
        p2 = a;
      }
    }
    bool operator==(const Edge& other) const
    {
      return p1.x == other.p1.x && p1.y == other.p1.y && p2.x == other.p2.x
             && p2.y == other.p2.y;
    }
  };
  struct EdgeHash
  {
    size_t operator()(const Edge& e) const
    {
      size_t h1 = GridHash{}(e.p1);
      size_t h2 = GridHash{}(e.p2);
      return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
  };
  std::unordered_map<Edge, int, EdgeHash> edge_counts;
  for (const auto& g : grids) {
    GridIndex v0 = {g.x, g.y};
    GridIndex v1 = {g.x + 1, g.y};
    GridIndex v2 = {g.x + 1, g.y + 1};
    GridIndex v3 = {g.x, g.y + 1};
    edge_counts[Edge(v0, v1)]++;
    edge_counts[Edge(v1, v2)]++;
    edge_counts[Edge(v2, v3)]++;
    edge_counts[Edge(v3, v0)]++;
  }
  std::unordered_multimap<GridIndex, GridIndex, GridHash> adj;
  for (const auto& [edge, count] : edge_counts) {
    if (count == 1) {
      adj.emplace(edge.p1, edge.p2);
      adj.emplace(edge.p2, edge.p1);
    }
  }
  if (adj.empty())
    throw std::runtime_error("No boundary found.");
  auto toVertexPoint = [&](const GridIndex& v) -> Point {
    Point  center = grid_.toPoint({v.x, v.y});
    double half   = grid_.step / 2.0;
    return Point{bg::get<0>(center) - half, bg::get<1>(center) - half};
  };
  std::vector<Point>                      path;
  std::unordered_set<GridIndex, GridHash> visited;
  GridIndex start_node = adj.begin()->first;
  GridIndex curr       = start_node;
  while (true) {
    path.push_back(toVertexPoint(curr));
    visited.insert(curr);
    GridIndex next;
    bool      found = false;
    auto      range = adj.equal_range(curr);
    for (auto it = range.first; it != range.second; ++it) {
      if (visited.find(it->second) == visited.end()) {
        next  = it->second;
        found = true;
        break;
      }
    }
    if (!found) {
      auto last_range = adj.equal_range(curr);
      for (auto it = last_range.first; it != last_range.second; ++it) {
        if (it->second.x == start_node.x && it->second.y == start_node.y
            && path.size() > 2) {
          break;
        }
      }
      break;
    };
    curr = next;
  }
  path.push_back(path.front());  
  Polygon poly;
  bg::assign_points(poly, path);
  bg::correct(poly);
  Polygon simplified_poly;
  bg::simplify(poly, simplified_poly, 0.01);
  return simplified_poly;
}
Polygon GlobalRouter::getTargetRegion(std::shared_ptr<Cell> cell,
                        double                margin = 1000.0) const
{
  auto free_grids    = findFreeGrids(cell, margin);
  auto target_region = gridsToPolygon(free_grids);
  return target_region;
}
std::vector<Polygon> GlobalRouter::toPolygonList(const MultiPolygon& multi) const
{
  std::vector<Polygon> result;
  for (const auto& poly : multi) {
    result.push_back(poly);
  }
  return result;
}
std::vector<Patch> GlobalRouter::createPatches(const Polygon& targetRegion,
                                 const Point&   ref,
                                 double         angle_deg,
                                 const Point&   pin_offset,
                                 double         r) const
{
  std::vector<Patch> patches;
  bg::model::box<Point> bbox;
  bg::envelope(targetRegion, bbox);
  double xmin = bbox.min_corner().x();
  double ymin = bbox.min_corner().y();
  double xmax = bbox.max_corner().x();
  double ymax = bbox.max_corner().y();
  double W   = xmax - xmin;
  double H   = ymax - ymin;
  double S   = std::max(W, H) * 20; 
  double eps = 1e-4;
  auto make_box = [](double x1, double y1, double x2, double y2) {
    Polygon p;
    bg::append(p.outer(), Point(x1, y1));
    bg::append(p.outer(), Point(x2, y1));
    bg::append(p.outer(), Point(x2, y2));
    bg::append(p.outer(), Point(x1, y2));
    bg::append(p.outer(), Point(x1, y1));
    bg::correct(p);
    return p;
  };
  auto patch0      = make_box(0, -eps, S, eps);
  auto rect_top    = make_box(2 * r, 2 * r, S, S);
  auto rect_bottom = make_box(2 * r, -S, S, -2 * r);
  MultiPolygon patch2_union;
  bg::union_(rect_top, rect_bottom, patch2_union);
  auto         full = make_box(-S, -S, S, S);
  MultiPolygon tmp;
  bg::difference(full, patch0, tmp);
  MultiPolygon patch4;
  bg::difference(tmp, patch2_union, patch4);
  auto transform = [&](const Polygon& poly) {
    Polygon out;
    double  rad = angle_deg * M_PI / 180.0;
    for (auto& p : poly.outer()) {
      double xr = p.x() * cos(rad) - p.y() * sin(rad);
      double yr = p.x() * sin(rad) + p.y() * cos(rad);
      bg::append(out.outer(),
                 Point(xr + ref.x() + pin_offset.x(),
                       yr + ref.y() + pin_offset.y()));
    }
    bg::correct(out);
    return out;
  };
  patches.push_back({transform(patch0), 0});
  for (auto& p : patch2_union)
    patches.push_back({transform(p), 2});
  for (auto& p : patch4)
    patches.push_back({transform(p), 4});
  return patches;
}
std::vector<std::pair<Polygon, std::vector<int>>> GlobalRouter::getSubRegions(
    std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>
            cell_pair,
    Polygon targetRegion,
    double  radius) const
{
  std::vector<std::vector<Patch>> all_patch_lists;
  for (auto pin1 : cell_pair.second) {
    auto net = pin1->getHostNet();
    if (!net)
      continue;
    auto all_pins = net->getPins();
    for (auto pin2 : all_pins) {
      if (pin2 == pin1 || pin2->getHostCell() == cell_pair.first)
        continue;
      std::cout << "----------------" << std::endl;
      std::cout << pin2->printInfo() << std::endl;
      double pin1_rot = pin1->getAbsoluteRotation();
      double pin2_rot = pin2->getAbsoluteRotation();
      double diff     = std::fmod(std::abs(pin1_rot - pin2_rot), 360.0);
      if (std::abs(diff - 180.0) > 1e-6)
        continue;
      auto offset = Point(-pin1->getPosition().x(), -pin1->getPosition().y());
      auto patches = createPatches(targetRegion,
                                   pin2->getAbsolutePosition(),
                                   pin2_rot,
                                   offset,
                                   radius);
      if (!patches.empty()) {
        all_patch_lists.push_back(patches);
      }
    }
  }
  if (all_patch_lists.empty()) {
    return {{targetRegion, {}}};
  }
  std::vector<RegionWithWeights> current_regions = {{targetRegion, {}}};
  for (const auto& patch_list : all_patch_lists) {
    std::vector<RegionWithWeights> new_regions;
    for (const auto& [cur_poly, cur_weights] : current_regions) {
      MultiPolygon covered_geom;
      for (const auto& [patch, w] : patch_list) {
        MultiPolygon inter;
        bg::intersection(cur_poly, patch, inter);
        auto polys = toPolygonList(inter);
        for (const auto& p : polys) {
          auto new_weights = cur_weights;
          new_weights.push_back(w);
          new_regions.push_back({p, new_weights});
          MultiPolygon tmp_covered;
          bg::union_(covered_geom, p, tmp_covered);
          covered_geom = std::move(tmp_covered);
        }
      }
    }
    current_regions = std::move(new_regions);
  }
  std::vector<std::pair<Polygon, std::vector<int>>> result;
  for (const auto& r : current_regions) {
    result.emplace_back(r.poly, r.weights);
  }
  return result;
}
void GlobalRouter::sortSubRegions(
    std::vector<std::pair<Polygon, std::vector<int>>>& subRegions)
{
  std::sort(
      subRegions.begin(), subRegions.end(), [](const auto& a, const auto& b) {
        int sum_a = std::accumulate(a.second.begin(), a.second.end(), 0);
        int sum_b = std::accumulate(b.second.begin(), b.second.end(), 0);
        if (sum_a != sum_b)
          return sum_a < sum_b;  
        return a.second < b.second;  
      });
}
Point GlobalRouter::getMedian(std::pair<std::shared_ptr<Cell>,
                          std::vector<std::shared_ptr<Pin>>> cell_pair) const
{
  std::vector<double> x_coords;
  std::vector<double> y_coords;
  for (auto pin1 : cell_pair.second) {
    auto net = pin1->getHostNet();
    if (!net)
      continue;
    auto all_pins = net->getPins();
    for (auto pin2 : all_pins) {
      if (pin2 == pin1 || pin2->getHostCell() == cell_pair.first)
        continue;
      auto pin2_pos = pin2->getAbsolutePosition();
      x_coords.push_back(pin2_pos.x());
      y_coords.push_back(pin2_pos.y());
    }
  }
  if (x_coords.empty() || y_coords.empty()) {
    return cell_pair.first->getPosition();
  }
  std::sort(x_coords.begin(), x_coords.end());
  std::sort(y_coords.begin(), y_coords.end());
  double median_x, median_y;
  size_t nx = x_coords.size();
  size_t ny = y_coords.size();
  if (nx % 2 == 1)
    median_x = x_coords[nx / 2];
  else
    median_x = (x_coords[nx / 2 - 1] + x_coords[nx / 2]) / 2.0;
  if (ny % 2 == 1)
    median_y = y_coords[ny / 2];
  else
    median_y = (y_coords[ny / 2 - 1] + y_coords[ny / 2]) / 2.0;
  return Point(median_x, median_y);
}
bool GlobalRouter::isInRegion(const Point& p, const Polygon& poly) const
{
  return bg::within(p, poly);
}
Point GlobalRouter::projectToRegion(const Point& p, const Polygon& poly) const
{
  bg::model::segment<Point> shortest_seg;
  bg::closest_points(p, poly, shortest_seg);
  return shortest_seg.second;
}
bool GlobalRouter::isBlocked(const std::shared_ptr<Cell>& cell) const
{
  if (!cell || !design_)
    return true;
  std::unordered_set<std::string> affected_net_names;
  for (auto& [net_name, net] : design_->getNets()) {
    for (auto pin : net->getPins()) {
      if (pin->getHostCell() == cell) {
        affected_net_names.insert(net_name);
        break;
      }
    }
  }
  std::unordered_map<GridIndex, int, GridHash> affected_path_usage;
  for (const auto& net_name : affected_net_names) {
    auto it = all_paths_.find(net_name);
    if (it != all_paths_.end()) {
      std::unordered_set<GridIndex, GridHash> visited_in_path;
      for (const auto& pt : it->second) {
        GridIndex idx = grid_.toIndex(pt);
        if (visited_in_path.insert(idx).second) {
          affected_path_usage[idx]++;
        }
      }
    }
  }
  auto   dim      = cell->getBoundingBox();
  auto   pos      = cell->getPosition();
  double cx       = pos.x();
  double cy       = pos.y();
  double origin_x = grid_.origin.get<0>();
  double origin_y = grid_.origin.get<1>();
  double step     = grid_.step;
  int min_x = static_cast<int>(
      std::floor((cx - dim.first / 2.0 - origin_x) / step));
  int min_y = static_cast<int>(
      std::floor((cy - dim.second / 2.0 - origin_y) / step));
  int max_x
      = static_cast<int>(std::ceil((cx + dim.first / 2.0 - origin_x) / step));
  int max_y = static_cast<int>(
      std::ceil((cy + dim.second / 2.0 - origin_y) / step));
  for (int ix = min_x; ix <= max_x; ++ix) {
    for (int iy = min_y; iy <= max_y; ++iy) {
      GridIndex idx{ix, iy};
  
      auto it = grid_.usage.find(idx);
      if (it != grid_.usage.end()) {
        int total_usage = it->second;
        auto aff_it = affected_path_usage.find(idx);
        if (aff_it != affected_path_usage.end()) {
          total_usage -= aff_it->second;
        }
        if (total_usage > 0) {
          return true;
        }
      }
    }
  }
  return false; 
}

bool GlobalRouter::rerouteCellNets(std::shared_ptr<Cell> moved_cell)
{
    using namespace std::chrono;
    auto total_start = high_resolution_clock::now();

    auto t1 = high_resolution_clock::now();
    std::unordered_set<std::string> affected_net_names;
    for (auto& [net_name, net] : design_->getNets()) {
        for (auto pin : net->getPins()) {
            if (pin->getHostCell() == moved_cell) {
                affected_net_names.insert(net_name);
                break;
            }
        }
    }
    auto t2 = high_resolution_clock::now();

    if (affected_net_names.empty()) {
        std::cout << "[Reroute] No nets affected. No routing needed.\n";
        return false;
    }

    std::unordered_map<std::string, std::vector<Point>> old_affected_paths;
    for (const auto& net_name : affected_net_names) {
        old_affected_paths[net_name] = all_paths_[net_name];
        all_paths_.erase(net_name);
    }

    auto t3 = high_resolution_clock::now();
    grid_.clear();
    addCellsToGrid();
    auto t4 = high_resolution_clock::now();

    auto t5 = high_resolution_clock::now();
    precalculatePinEscapes();
    auto t6 = high_resolution_clock::now();

    auto t7 = high_resolution_clock::now();
    for (const auto& [net_name, path] : all_paths_) {
        std::unordered_set<GridIndex, GridHash> visited_gcells;
        for (const auto& pt : path) {
            GridIndex idx = grid_.toIndex(pt);
            if (visited_gcells.insert(idx).second) {
                grid_.usage[idx] += 1;
            }
        }
    }
    auto t8 = high_resolution_clock::now();

    auto t9 = high_resolution_clock::now();
    std::unordered_map<std::string, std::vector<Point>> new_affected_paths;
    auto nets = design_->getNets();
    for (const auto& net_name : affected_net_names) {
        auto net_it = nets.find(net_name);
        if (net_it != nets.end()) {
            if (!routeSingleNet(net_name, net_it->second)) {
                return false;
            }
            new_affected_paths[net_name] = all_paths_[net_name];
        }
    }
    auto t10 = high_resolution_clock::now();

    auto t11 = high_resolution_clock::now();
    if (ilg_) {
        ilg_->incrementalUpdate(moved_cell, old_affected_paths,
                                new_affected_paths, all_paths_, grid_);
    }
    auto t12 = high_resolution_clock::now();

    auto total_end = high_resolution_clock::now();
    auto duration = [](auto start, auto end) {
        return duration_cast<microseconds>(end - start).count() / 1000.0;
    };

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[Reroute Timing] Cell: " << moved_cell->getName()
              << ", Affected nets: " << affected_net_names.size() << "\n";
    std::cout << "  Phase1 (find nets):        " << duration(t1, t2) << " ms\n";
    std::cout << "  Phase2 (addCellsToGrid):   " << duration(t3, t4) << " ms\n";
    std::cout << "  Phase3 (precalc escapes):  " << duration(t5, t6) << " ms\n";
    std::cout << "  Phase4 (restore paths):    " << duration(t7, t8) << " ms\n";
    std::cout << "  Phase5 (re-route nets):    " << duration(t9, t10) << " ms\n";
    std::cout << "  Phase6 (ILG update):       " << duration(t11, t12) << " ms\n";
    std::cout << "  Total reroute time:        " << duration(total_start, total_end) << " ms\n\n";

    return true;
}

