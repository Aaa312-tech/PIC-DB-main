#include "fast_route.h"
inline Direction degreeToDirection(double angle)
{
  double normalized_angle = std::fmod(angle, 360.0);
  if (normalized_angle < 0.0)
    normalized_angle += 360.0;
  const double epsilon = 1e-4;
  if (std::abs(normalized_angle - 0.0) < epsilon
      || std::abs(normalized_angle - 360.0) < epsilon)
    return Direction::EAST;
  else if (std::abs(normalized_angle - 90.0) < epsilon)
    return Direction::NORTH;
  else if (std::abs(normalized_angle - 180.0) < epsilon)
    return Direction::WEST;
  else if (std::abs(normalized_angle - 270.0) < epsilon)
    return Direction::SOUTH;
  return Direction::NONE;
}

inline Direction getAxis(Direction dir)
{
  if (dir == Direction::NORTH || dir == Direction::SOUTH)
    return Direction::VERTICAL;
  if (dir == Direction::EAST || dir == Direction::WEST)
    return Direction::HORIZONTAL;
  return Direction::NONE;
}

inline Direction reverseDir(Direction dir)
{
  if (dir == Direction::NORTH)
    return Direction::SOUTH;
  if (dir == Direction::SOUTH)
    return Direction::NORTH;
  if (dir == Direction::EAST)
    return Direction::WEST;
  if (dir == Direction::WEST)
    return Direction::EAST;
  return Direction::NONE;
}


void GlobalRouter::addCellsToGrid()
{
  double lx_design = bg::get<0>(grid_.origin);
  double ly_design = bg::get<1>(grid_.origin);
  double step      = grid_.step;
  for (auto& [cell_name, cell] : design_->getCells()) {
    auto shape = cell->getShape();
    auto rect  = std::dynamic_pointer_cast<Rect>(shape);
    if (!rect)
      continue;
    auto   dimension = cell->getBoundingBox();
    double cx        = cell->getPosition().x();
    double cy        = cell->getPosition().y();
    double cell_lx = cx - dimension.first / 2;
    double cell_ly = cy - dimension.second / 2;
    double cell_ux = cx + dimension.first / 2;
    double cell_uy = cy + dimension.second / 2;
    int min_ix = static_cast<int>(std::floor((cell_lx - lx_design) / step));
    int min_iy = static_cast<int>(std::floor((cell_ly - ly_design) / step));
    int max_ix = static_cast<int>(std::ceil((cell_ux - lx_design) / step));
    int max_iy = static_cast<int>(std::ceil((cell_uy - ly_design) / step));
    for (int ix = min_ix; ix <= max_ix; ++ix) {
      for (int iy = min_iy; iy <= max_iy; ++iy) {
        grid_.usage[{ix, iy}] += 2;
      }
    }
  }
}

void GlobalRouter::precalculatePinEscapes()
{
  escape_map_.clear();
  std::map<std::shared_ptr<Cell>,
           std::map<Direction, std::vector<std::shared_ptr<Pin>>>>
      cell_pin_groups;
  for (auto& [net_name, net] : design_->getNets()) {
    for (auto pin : net->getPins()) {
      auto      cell = pin->getHostCell();
      Direction dir  = degreeToDirection(pin->getAbsoluteRotation());
      if (cell)
        cell_pin_groups[cell][dir].push_back(pin);
      else {
        Point p          = pin->getAbsolutePosition();
        escape_map_[pin] = {grid_.toIndex(p), p, dir};
      }
    }
  }
  double margin = grid_.step * 1.5;
  for (auto& [cell, dir_groups] : cell_pin_groups) {
    auto   dim   = cell->getBoundingBox();
    double cx    = cell->getPosition().x();
    double cy    = cell->getPosition().y();
    double min_x = cx - dim.first / 2.0;
    double max_x = cx + dim.first / 2.0;
    double min_y = cy - dim.second / 2.0;
    double max_y = cy + dim.second / 2.0;
    for (auto& [dir, pins] : dir_groups) {
      if (dir == Direction::EAST || dir == Direction::WEST) {
        std::sort(pins.begin(),
                  pins.end(),
                  [](std::shared_ptr<Pin> a, std::shared_ptr<Pin> b) {
                    return bg::get<1>(a->getAbsolutePosition())
                           < bg::get<1>(b->getAbsolutePosition());
                  });
      } else {
        std::sort(pins.begin(),
                  pins.end(),
                  [](std::shared_ptr<Pin> a, std::shared_ptr<Pin> b) {
                    return bg::get<0>(a->getAbsolutePosition())
                           < bg::get<0>(b->getAbsolutePosition());
                  });
      }
      std::vector<int> transverse_indices;
      for (size_t i = 0; i < pins.size(); ++i) {
        auto   pin      = pins[i];
        Point  p        = pin->getAbsolutePosition();
        double escape_x = bg::get<0>(p);
        double escape_y = bg::get<1>(p);
        if (dir == Direction::EAST)
          escape_x = max_x + margin;
        else if (dir == Direction::WEST)
          escape_x = min_x - margin;
        else if (dir == Direction::NORTH)
          escape_y = max_y + margin;
        else if (dir == Direction::SOUTH)
          escape_y = min_y - margin;
        GridIndex ideal_idx = grid_.toIndex({escape_x, escape_y});
        int       t_idx = (dir == Direction::EAST || dir == Direction::WEST)
                              ? ideal_idx.y
                              : ideal_idx.x;
        if (i > 0 && t_idx <= transverse_indices.back()) {
          t_idx = transverse_indices.back() + 1;
        }
        transverse_indices.push_back(t_idx);
        GridIndex final_idx = ideal_idx;
        if (dir == Direction::EAST || dir == Direction::WEST)
          final_idx.y = t_idx;
        else
          final_idx.x = t_idx;
        Point final_physical = grid_.toPoint(final_idx);
        if (dir == Direction::EAST || dir == Direction::WEST)
          bg::set<0>(final_physical, escape_x);
        else
          bg::set<1>(final_physical, escape_y);
        escape_map_[pin] = {final_idx, final_physical, dir};
      }
    }
  }
}

std::vector<Point> GlobalRouter::fineGrainedRoute(const std::vector<GridIndex>& coarse_path,
                                    const PinEscapeInfo& start_escape,
                                    const PinEscapeInfo& end_escape) const
{
  double min_x = 1e9, min_y = 1e9, max_x = -1e9, max_y = -1e9;
  for (const auto& idx : coarse_path) {
    Point p = grid_.toPoint(idx);
    min_x   = std::min(min_x, bg::get<0>(p));
    min_y   = std::min(min_y, bg::get<1>(p));
    max_x   = std::max(max_x, bg::get<0>(p));
    max_y   = std::max(max_y, bg::get<1>(p));
  }
  double margin = grid_.step * 3;
  min_x -= margin;
  min_y -= margin;
  max_x += margin;
  max_y += margin;
  double      fine_step = grid_.step / 5.0;
  RoutingGrid fine_grid(fine_step, Point(min_x, min_y), 1);
  for (auto& [cell_name, cell] : design_->getCells()) {
    auto shape = cell->getShape();
    auto rect  = std::dynamic_pointer_cast<Rect>(shape);
    if (!rect)
      continue;
    auto   dimension = cell->getBoundingBox();
    double cell_lx   = cell->getPosition().x() - dimension.first / 2;
    double cell_ly   = cell->getPosition().y() - dimension.second / 2;
    double cell_ux   = cell->getPosition().x() + dimension.first / 2;
    double cell_uy   = cell->getPosition().y() + dimension.second / 2;
    if (cell_ux >= min_x && cell_lx <= max_x && cell_uy >= min_y
        && cell_ly <= max_y) {
      double c_min_x = std::max(min_x, cell_lx);
      double c_min_y = std::max(min_y, cell_ly);
      double c_max_x = std::min(max_x, cell_ux);
      double c_max_y = std::min(max_y, cell_uy);
      int min_ix
          = static_cast<int>(std::floor((c_min_x - min_x) / fine_step));
      int min_iy
          = static_cast<int>(std::floor((c_min_y - min_y) / fine_step));
      int max_ix = static_cast<int>(std::ceil((c_max_x - min_x) / fine_step));
      int max_iy = static_cast<int>(std::ceil((c_max_y - min_y) / fine_step));
      for (int ix = min_ix; ix <= max_ix; ++ix) {
        for (int iy = min_iy; iy <= max_iy; ++iy) {
          fine_grid.usage[{ix, iy}] = 1;
        }
      }
    }
  }
  GridIndex start_idx = fine_grid.toIndex(start_escape.physical_pos);
  GridIndex end_idx   = fine_grid.toIndex(end_escape.physical_pos);
  Direction start_dir = start_escape.escape_dir;
  Direction end_dir   = reverseDir(end_escape.escape_dir);
  const double FINE_BEND_PENALTY = 2.0;
  const double CROSSING_PENALTY  = 50.0;
  using SearchNode = std::tuple<double, GridIndex, Direction>;
  std::priority_queue<SearchNode,
                      std::vector<SearchNode>,
                      std::greater<SearchNode>>
      open_list;
  std::unordered_map<GridIndex, GridIndex, GridHash> parent_map;
  std::unordered_map<GridIndex, double, GridHash>    g_score;
  std::unordered_map<GridIndex, Direction, GridHash> arrival_dir;
  open_list.push({0.0, start_idx, Direction::NONE});
  g_score[start_idx]     = 0.0;
  arrival_dir[start_idx] = Direction::NONE;
  int fine_grid_w = static_cast<int>(std::ceil((max_x - min_x) / fine_step));
  int fine_grid_h = static_cast<int>(std::ceil((max_y - min_y) / fine_step));
  bool found = false;
  while (!open_list.empty()) {
    auto [f, current, in_dir] = open_list.top();
    open_list.pop();
    if (current == end_idx) {
      found = true;
      break;
    }
    struct
    {
      GridIndex idx;
      Direction dir;
    } neighbors[4] = {{{current.x + 1, current.y}, Direction::EAST},
                      {{current.x - 1, current.y}, Direction::WEST},
                      {{current.x, current.y + 1}, Direction::NORTH},
                      {{current.x, current.y - 1}, Direction::SOUTH}};
    for (auto& neighbor : neighbors) {
      GridIndex next     = neighbor.idx;
      Direction move_dir = neighbor.dir;
      if (next.x < 0 || next.y < 0 || next.x > fine_grid_w
          || next.y > fine_grid_h)
        continue;
      if (fine_grid.isFull(next) && next != end_idx && next != start_idx)
        continue;
      if (current == start_idx && start_dir != Direction::NONE
          && move_dir != start_dir)
        continue;
      if (next == end_idx && end_dir != Direction::NONE
          && move_dir != end_dir)
        continue;
      double    edge_cost         = 1.0;
      GridIndex coarse_mapped_idx = grid_.toIndex(fine_grid.toPoint(next));
      auto      coarse_it         = grid_.usage.find(coarse_mapped_idx);
      if (coarse_it != grid_.usage.end() && coarse_it->second > 0)
        edge_cost += CROSSING_PENALTY;
      Direction in_axis   = getAxis(in_dir);
      Direction move_axis = getAxis(move_dir);
      if (in_axis != Direction::NONE && in_axis != move_axis)
        edge_cost += FINE_BEND_PENALTY;
      double tentative_g = g_score[current] + edge_cost;
      if (g_score.find(next) == g_score.end()
          || tentative_g < g_score[next]) {
        g_score[next]     = tentative_g;
        arrival_dir[next] = move_dir;
        double h
            = std::abs(next.x - end_idx.x) + std::abs(next.y - end_idx.y);
        open_list.push({tentative_g + h, next, move_dir});
        parent_map[next] = current;
      }
    }
  }
  std::vector<Point> fine_path;
  if (found) {
    GridIndex curr = end_idx;
    while (curr != start_idx) {
      fine_path.push_back(fine_grid.toPoint(curr));
      curr = parent_map[curr];
    }
    fine_path.push_back(fine_grid.toPoint(start_idx));
    std::reverse(fine_path.begin(), fine_path.end());
  } else {
    for (const auto& idx : coarse_path)
      fine_path.push_back(grid_.toPoint(idx));
  }
  return fine_path;
}

bool GlobalRouter::routeSingleNet(const std::string& net_name, std::shared_ptr<Net> net)
{
  auto pins = net->getPins();
  if (pins.size() < 2)
    return false;
  auto start_pin = pins[0];
  auto end_pin   = pins[1];
  auto start_escape = escape_map_[start_pin];
  auto end_escape   = escape_map_[end_pin];
  GridIndex start_idx = start_escape.coarse_gcell_idx;
  GridIndex end_idx   = end_escape.coarse_gcell_idx;
  Direction start_dir = start_escape.escape_dir;
  Direction end_dir   = reverseDir(end_escape.escape_dir);
  using SearchNode = std::tuple<double, GridIndex, Direction>;
  std::priority_queue<SearchNode,
                      std::vector<SearchNode>,
                      std::greater<SearchNode>>
                                                     open_list;
  std::unordered_map<GridIndex, GridIndex, GridHash> parent_map;
  std::unordered_map<GridIndex, double, GridHash>    g_score;
  open_list.push({0.0, start_idx, Direction::NONE});
  g_score[start_idx] = 0.0;
  const double BEND_PENALTY = 5.0;
  bool         found        = false;
  while (!open_list.empty()) {
    auto [f, current, in_dir] = open_list.top();
    open_list.pop();
    if (current == end_idx) {
      found = true;
      break;
    }
    struct
    {
      GridIndex idx;
      Direction dir;
    } neighbors[4] = {{{current.x + 1, current.y}, Direction::EAST},
                      {{current.x - 1, current.y}, Direction::WEST},
                      {{current.x, current.y + 1}, Direction::NORTH},
                      {{current.x, current.y - 1}, Direction::SOUTH}};
    for (auto& neighbor : neighbors) {
      GridIndex next     = neighbor.idx;
      Direction move_dir = neighbor.dir;
      if (next.x < 0 || next.y < 0 || next.x > (ux_ - lx_) / grid_.step
          || next.y > (uy_ - ly_) / grid_.step)
        continue;
      if (current == start_idx && start_dir != Direction::NONE
          && move_dir != start_dir)
        continue;
      if (next == end_idx && end_dir != Direction::NONE
          && move_dir != end_dir)
        continue;
      double edge_cost = grid_.getCost(next);
      Direction in_axis   = getAxis(in_dir);
      Direction move_axis = getAxis(move_dir);
      if (in_axis != Direction::NONE && in_axis != move_axis)
        edge_cost += BEND_PENALTY;
      double tentative_g = g_score[current] + edge_cost;
      if (g_score.find(next) == g_score.end()
          || tentative_g < g_score[next]) {
        g_score[next] = tentative_g;
        double h
            = std::abs(next.x - end_idx.x) + std::abs(next.y - end_idx.y);
        open_list.push({tentative_g + h, next, move_dir});
        parent_map[next] = current;
      }
    }
  }
  if (found) {
    std::vector<GridIndex> coarse_path_idx;
    GridIndex              curr = end_idx;
    while (curr != start_idx) {
      coarse_path_idx.push_back(curr);
      curr = parent_map[curr];
    }
    coarse_path_idx.push_back(start_idx);
    std::reverse(coarse_path_idx.begin(), coarse_path_idx.end());
    bool needs_fine_routing = false;
    for (const auto& idx : coarse_path_idx) {
      if (idx == start_idx || idx == end_idx)
        continue;
      if (grid_.usage[idx] > 0) {
        needs_fine_routing = true;
        break;
      }
    }
    std::vector<Point> a_star_path;
    if (needs_fine_routing) {
      a_star_path
          = fineGrainedRoute(coarse_path_idx, start_escape, end_escape);
    } else {
      for (const auto& idx : coarse_path_idx) {
        a_star_path.push_back(grid_.toPoint(idx));
        grid_.usage[idx] += 1;
      }
    }
    std::vector<Point> final_path;
    final_path.push_back(start_pin->getAbsolutePosition());
    if (!isPointEqual(final_path.back(), start_escape.physical_pos))
      final_path.push_back(start_escape.physical_pos);
    for (const auto& p : a_star_path) {
      if (final_path.empty() || !isPointEqual(final_path.back(), p))
        final_path.push_back(p);
    }
    if (!isPointEqual(final_path.back(), end_escape.physical_pos))
      final_path.push_back(end_escape.physical_pos);
    if (!isPointEqual(final_path.back(), end_pin->getAbsolutePosition()))
      final_path.push_back(end_pin->getAbsolutePosition());
    all_paths_[net_name] = final_path;
    return true;
  } else {
    std::cout << "Routing failed for net: " << net_name << std::endl;
    return false;
  }
}

GlobalRouter::GlobalRouter(std::shared_ptr<Design> design,
             double                  coarse_step,
             int                     capacity)
    : design_(design)
{
  auto rect = std::dynamic_pointer_cast<Rect>(design_->getShape());
  lx_       = rect->getShape().min_corner().x();
  ly_       = rect->getShape().min_corner().y();
  ux_       = rect->getShape().max_corner().x();
  uy_       = rect->getShape().max_corner().y();
  Point low_corner = {lx_, ly_};
  grid_            = RoutingGrid(coarse_step, low_corner, capacity);
}
bool GlobalRouter::checkEscapeMapCollisions() const
{
  bool has_collision = false;
  std::unordered_map<
      GridIndex,
      std::vector<std::pair<std::shared_ptr<Pin>, PinEscapeInfo>>,
      GridHash>
      grouped_escapes;
  for (const auto& [pin, info] : escape_map_) {
    grouped_escapes[info.coarse_gcell_idx].push_back({pin, info});
  }
  std::cout << "\n[Pre-Check] Scanning escape map for pin collisions...\n";
  for (const auto& [gidx, escapes] : grouped_escapes) {
    if (escapes.size() > 1) {
      for (size_t i = 0; i < escapes.size(); ++i) {
        for (size_t j = i + 1; j < escapes.size(); ++j) {
          if (escapes[i].second == escapes[j].second) {
            auto pin1 = escapes[i].first;
            auto pin2 = escapes[j].first;
            std::string cell1_name = pin1->getHostCell()
                                         ? pin1->getHostCell()->getName()
                                         : "TopLevel";
            std::string cell2_name = pin2->getHostCell()
                                         ? pin2->getHostCell()->getName()
                                         : "TopLevel";
            std::cout << "  [!] Collision Detected:\n"
                      << "      - Pin A: " << pin1->getName()
                      << " (Cell: " << cell1_name << ")\n"
                      << "      - Pin B: " << pin2->getName()
                      << " (Cell: " << cell2_name << ")\n"
                      << "      - Conflict At: Grid(" << gidx.x << ", "
                      << gidx.y << "), "
                      << "Physical Pos("
                      << bg::get<0>(escapes[i].second.physical_pos) << ", "
                      << bg::get<1>(escapes[i].second.physical_pos) << ")\n";
            has_collision = true;
          }
        }
      }
    }
  }
  if (!has_collision) {
    std::cout
        << "  [+] Escape map is clean. No collisions found. Safe to route.\n";
  }
  return has_collision;
}
void GlobalRouter::setInsertionLossGraph(std::shared_ptr<InsertionLossGraph> ilg)
{
  ilg_ = ilg;
}
const std::unordered_map<std::string, std::vector<Point>>& GlobalRouter::getAllPaths() const
{
  return all_paths_;
}
const RoutingGrid& GlobalRouter::getRouterGrid() const { return grid_; }
void GlobalRouter::initialRoute()
{
  all_paths_.clear();
  grid_.clear();
  addCellsToGrid();
  precalculatePinEscapes();
  for (auto& [net_name, net] : design_->getNets()) {
    routeSingleNet(net_name, net);
  }
}

RoutingInfo GlobalRouter::getRoutingInfo() const
{
  struct RoutingInfo info;
  info.grid_       = grid_;
  info.all_paths_  = all_paths_;
  info.escape_map_ = escape_map_;
  info.ilg_        = std::make_shared<InsertionLossGraph>(*ilg_);
  return info;
}
void GlobalRouter::setRoutingInfo(const RoutingInfo& info)
{
  grid_       = info.grid_;
  all_paths_  = info.all_paths_;
  escape_map_ = info.escape_map_;
  ilg_        = std::make_shared<InsertionLossGraph>(*info.ilg_);
}
void GlobalRouter::loadRoutingInfo(const RoutingInfo& info)
{
  grid_       = info.grid_;
  all_paths_  = info.all_paths_;
  escape_map_ = info.escape_map_;
  ilg_        = std::make_shared<InsertionLossGraph>(*info.ilg_);
}
void GlobalRouter::exportToSVG(
    const std::string& filename,
    const std::vector<
        std::pair<std::shared_ptr<Cell>, std::vector<std::shared_ptr<Pin>>>>&
        critical_cells,
    const Polygon&                                           poly,
    const std::vector<std::pair<Polygon, std::vector<int>>>& subRegions,
    const std::vector<Point>&                                X) const
{
  std::ofstream out(filename);
  if (!out.is_open())
    return;
  double margin = 50.0;
  double width  = ux_ - lx_ + 2 * margin;
  double height = uy_ - ly_ + 2 * margin;
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
  out << "<svg width=\"" << width << "\" height=\"" << height
      << "\" viewBox=\"" << (lx_ - margin) << " " << (ly_ - margin) << " "
      << width << " " << height
      << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
  out << "  <rect x=\"" << (lx_ - margin) << "\" y=\"" << (ly_ - margin)
      << "\" width=\"" << width << "\" height=\"" << height
      << "\" fill=\"#f0f8ff\" />\n";
  for (const auto& [idx, usage] : grid_.usage) {
    if (usage <= 0)
      continue;
    Point  p  = grid_.toPoint(idx);
    double gx = bg::get<0>(p) - grid_.step / 2.0;
    double gy = bg::get<1>(p) - grid_.step / 2.0;
    std::string fill_color;
    if (usage == 1)
      fill_color = "#ffd8b1";
    else if (usage == 2)
      fill_color = "#ffb366";
    else if (usage == 3)
      fill_color = "#ff8000";
    else
      fill_color = "#cc0000";
    out << "  <rect x=\"" << gx << "\" y=\"" << gy << "\" width=\""
        << grid_.step << "\" height=\"" << grid_.step << "\" fill=\""
        << fill_color
        << "\" stroke=\"#cccccc\" stroke-width=\"0.5\" opacity=\"0.8\" />\n";
  }
  for (const auto& [cell_name, cell] : design_->getCells()) {
    auto   dimension = cell->getBoundingBox();
    double llx       = cell->getPosition().x() - dimension.first / 2;
    double lly       = cell->getPosition().y() - dimension.second / 2;
    out << "  <rect x=\"" << llx << "\" y=\"" << lly << "\" width=\""
        << dimension.first << "\" height=\"" << dimension.second
        << "\" fill=\"none\" stroke=\"#000080\" stroke-width=\"2\" "
           "stroke-dasharray=\"5,5\" />\n";
    out << "  <text x=\"" << llx + 5 << "\" y=\"" << lly + 15
        << "\" font-family=\"Arial\" font-size=\"10\" fill=\"#000080\">"
        << cell_name << "</text>\n";
  }
  for (const auto& cell : critical_cells) {
    if (!cell.first)
      continue;
    auto   dimension = cell.first->getBoundingBox();
    double llx       = cell.first->getPosition().x() - dimension.first / 2;
    double lly       = cell.first->getPosition().y() - dimension.second / 2;
    out << "  <rect x=\"" << llx << "\" y=\"" << lly << "\" width=\""
        << dimension.first << "\" height=\"" << dimension.second
        << "\" fill=\"rgba(255, 0, 0, 0.15)\" "
        << "stroke=\"#FF0000\" stroke-width=\"5\" rx=\"2\" ry=\"2\" />\n";
    out << "  <text x=\"" << llx + 5 << "\" y=\"" << lly + 15
        << "\" font-family=\"Arial\" font-size=\"10\" font-weight=\"bold\" "
           "fill=\"#FF0000\">"
        << "CRITICAL"
        << "</text>\n";
  }
  if (!poly.outer().empty()) {
    out << "  <polygon points=\"";
    for (const auto& pt : poly.outer()) {
      out << bg::get<0>(pt) << "," << bg::get<1>(pt) << " ";
    }
    out << "\" fill=\"rgba(144, 238, 144, 0.5)\" stroke=\"#228B22\" "
           "stroke-width=\"2\" />\n";
  }
  if (!subRegions.empty()) {
    std::vector<int> sumBends;
    sumBends.reserve(subRegions.size());
    int minSum = std::numeric_limits<int>::max();
    int maxSum = std::numeric_limits<int>::min();
    for (const auto& region : subRegions) {
      int sum
          = std::accumulate(region.second.begin(), region.second.end(), 0);
      sumBends.push_back(sum);
      if (sum < minSum)
        minSum = sum;
      if (sum > maxSum)
        maxSum = sum;
    }
    auto getLabelPoint = [](const Polygon& poly) -> Point {
      Point pt;
      bg::point_on_surface(poly, pt);
      return pt;
    };
    for (size_t i = 0; i < subRegions.size(); ++i) {
      const auto& poly = subRegions[i].first;
      int         sum  = sumBends[i];
      if (!bg::is_valid(poly)) {
        Polygon corrected = poly;
        bg::correct(corrected);
        if (!bg::is_valid(corrected)) {
          std::cout << "corrected error" << std::endl;
          continue; 
        }
        std::cout << "error" << std::endl;
        continue;
      }
      double t = 0.5;  
      if (minSum != maxSum) {
        t = static_cast<double>(sum - minSum) / (maxSum - minSum);
      }
      int green = static_cast<int>(50 + t * 180);  // 50 ~ 230
      int red   = static_cast<int>(20 + t * 60);   // 20 ~ 80
      int blue  = static_cast<int>(20 + t * 60);   // 20 ~ 80
      out << "  <polygon points=\"";
      for (const auto& pt : poly.outer()) {
        out << bg::get<0>(pt) << "," << bg::get<1>(pt) << " ";
      }
      out << "\" fill=\"rgba(" << red << ", " << green << ", " << blue
          << ", 0.6)\" stroke=\"#004d00\" stroke-width=\"1.5\" />\n";
      Point labelPoint = getLabelPoint(poly);
      double area     = bg::area(poly);
      int    fontSize = 16;  
      if (area > 0) {
        double size = std::sqrt(area) * 0.5;
        if (size < 12)
          size = 12;
        if (size > 24)
          size = 24;
        fontSize = static_cast<int>(size);
      }
      out << "  <text x=\"" << bg::get<0>(labelPoint) << "\" y=\""
          << bg::get<1>(labelPoint) << "\" font-family=\"Arial\" font-size=\""
          << fontSize << "\""
          << " fill=\"white\" stroke=\"black\" stroke-width=\"0.8\""
          << " text-anchor=\"middle\" font-weight=\"bold\""
          << " paint-order=\"stroke fill\">" << sum << "</text>\n";
    }
  }
  if (!X.empty()) {
    for (const auto& pt : X) {
      out << "  <circle cx=\"" << bg::get<0>(pt) << "\" cy=\""
          << bg::get<1>(pt)
          << "\" r=\"5\" fill=\"#FF0000\" stroke=\"#000000\" "
             "stroke-width=\"1.5\" />\n";
    }
  }
  std::vector<std::string> colors
      = {"#2ca02c", "#1f77b4", "#9467bd", "#e377c2", "#17becf"};
  int color_idx = 0;
  for (const auto& [net_name, path] : all_paths_) {
    if (path.empty())
      continue;
    out << "  <polyline points=\"";
    for (const auto& pt : path)
      out << bg::get<0>(pt) << "," << bg::get<1>(pt) << " ";
    out << "\" fill=\"none\" stroke=\"" << colors[color_idx % colors.size()]
        << "\" stroke-width=\"3\" stroke-linejoin=\"round\" />\n";
    color_idx++;
  }
  for (const auto& [net_name, net] : design_->getNets()) {
    for (const auto& pin : net->getPins()) {
      double px = pin->getAbsolutePosition().x();
      double py = pin->getAbsolutePosition().y();
      out << "  <circle cx=\"" << px << "\" cy=\"" << py
          << "\" r=\"3\" fill=\"#d62728\" stroke=\"white\" "
             "stroke-width=\"1\" />\n";
    }
  }
  out << "</svg>\n";
  out.close();
}



#include "ilg.h"



void GlobalRouter::analyzeDatapathLosses(int K)
{
  ilg_->analyzeDatapathLosses(K);
}