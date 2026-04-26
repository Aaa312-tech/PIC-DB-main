#include "init_place.h"

void printGraphDebugInfo(
    const std::vector<std::shared_ptr<Cell>>&                     startpoints,
    const std::unordered_set<std::shared_ptr<Cell>>&              endpoints,
    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs,
    const std::unordered_map<std::shared_ptr<Pin>,
                             std::vector<std::shared_ptr<Pin>>>&  graph)
{
  std::cout << "\n========== DEBUG: GRAPH INFO ==========\n";

  std::cout << "\n--- Startpoints (" << startpoints.size() << ") ---\n";
  for (const auto& sp : startpoints) {
    std::cout << "  Cell: " << sp->getName() << "\n";
  }

  std::cout << "\n--- Endpoints (" << endpoints.size() << ") ---\n";
  for (const auto& ep : endpoints) {
    std::cout << "  Cell: " << ep->getName() << "\n";
  }

  std::cout << "\n--- Pin Directions ---\n";
  if (pin_dirs.empty()) {
    std::cout << "  [Warning] No pin directions inferred!\n";
  }
  for (const auto& [pin, dir] : pin_dirs) {
    std::string dir_str = (dir == PinDirection::INPUT)    ? "INPUT"
                          : (dir == PinDirection::OUTPUT) ? "OUTPUT"
                                                          : "UNKNOWN";
    std::cout << "  " << pin->getHostCell()->getName() << "/" << pin->getName()
              << " -> " << dir_str << "\n";
  }

  std::cout << "\n--- Graph Edges (Source Pin -> Destination Pins) ---\n";
  if (graph.empty()) {
    std::cout << "  [Warning] Graph is completely empty!\n";
  }
  for (const auto& [src_pin, dest_pins] : graph) {
    std::cout << "  " << src_pin->getHostCell()->getName() << "/"
              << src_pin->getName() << " drives:\n";
    if (dest_pins.empty()) {
      std::cout << "    [NONE - Dead End]\n";
    } else {
      for (const auto& dest_pin : dest_pins) {
        std::cout << "    -> " << dest_pin->getHostCell()->getName() << "/"
                  << dest_pin->getName() << "\n";
      }
    }
  }
  std::cout << "=======================================\n\n";
}

void dumpLevelizedCellsToDot(
    const std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs,
    const std::string& filename)
{
  std::ofstream out(filename);
  if (!out.is_open()) {
    std::cerr << "[Error] Cannot open " << filename << " for writing.\n";
    return;
  }

  out << "digraph LevelizedCells {\n";
  out << "  rankdir=LR;\n";   
  out << "  nodesep=0.5;\n"; 
  out << "  ranksep=1.5;\n"; 
  out << "  splines=line;\n"; 
  out << "  node [shape=box, style=\"filled,rounded\", fillcolor=aliceblue, "
         "fontname=Helvetica];\n";

  for (size_t l = 0; l < levelized_cells.size(); ++l) {
    if (levelized_cells[l].empty())
      continue;

    out << "  subgraph cluster_layer_" << l << " {\n";
    out << "    label=\"Layer " << l << "\";\n";
    out << "    style=dashed;\n";
    out << "    color=gray;\n";

    out << "    { rank=same;\n";
    for (const auto& cell : levelized_cells[l]) {
      out << "      \"" << cell->getName() << "\";\n";
    }
    out << "    }\n";

    out << "    edge [style=invis, weight=100];\n";
    for (size_t i = 0; i + 1 < levelized_cells[l].size(); ++i) {
      out << "    \"" << levelized_cells[l][i]->getName() << "\" -> \""
          << levelized_cells[l][i + 1]->getName() << "\";\n";
    }
    out << "  }\n";
  }

  out << "  // Data paths\n";
  out << "  edge [style=solid, weight=1, color=black];\n";
  std::unordered_set<std::shared_ptr<Net>> visited_nets;

  for (const auto& layer : levelized_cells) {
    for (const auto& cell : layer) {
      for (const auto& [pin_name, pin] : cell->getPins()) {
        auto net = pin->getHostNet();
        if (!net || visited_nets.count(net))
          continue;
        visited_nets.insert(net);

        std::shared_ptr<Cell>              driver = nullptr;
        std::vector<std::shared_ptr<Cell>> receivers;

        for (const auto& net_pin : net->getPins()) {
          auto it = pin_dirs.find(net_pin);
          if (it != pin_dirs.end()) {
            if (it->second == PinDirection::OUTPUT) {
              driver = net_pin->getHostCell();
            } else if (it->second == PinDirection::INPUT) {
              receivers.push_back(net_pin->getHostCell());
            }
          }
        }

        if (driver) {
          for (const auto& recv : receivers) {
            if (driver != recv) {
              out << "  \"" << driver->getName() << "\" -> \""
                  << recv->getName() << "\";\n";
            }
          }
        }
      }
    }
  }

  out << "}\n";
  out.close();
  std::cout << "[Info] Graphviz DOT file exported to " << filename << "\n";
  std::cout << "[Info] To render an image, run: dot -Tpng " << filename
            << " -o graph.png\n";
}

void disperseInternalCellsTopologically(
    const std::shared_ptr<Design>&                         design,
    const std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells)
{
  int num_levels = levelized_cells.size();

  if (num_levels <= 2) {
    std::cout << "[Info] No internal cells to disperse.\n";
    return;
  }

  double min_x = 0.0, max_x = 1000.0;
  double min_y = 0.0, max_y = 1000.0;

  auto design_shape = design->getShape();
  if (design_shape != nullptr) {
    auto points = design_shape->getPoints();
    if (!points.empty()) {
      min_x = std::numeric_limits<double>::max();
      max_x = std::numeric_limits<double>::lowest();
      min_y = std::numeric_limits<double>::max();
      max_y = std::numeric_limits<double>::lowest();

      for (const auto& pt : points) {
        if (pt.x() < min_x)
          min_x = pt.x();
        if (pt.x() > max_x)
          max_x = pt.x();
        if (pt.y() < min_y)
          min_y = pt.y();
        if (pt.y() > max_y)
          max_y = pt.y();
      }
    }
  }

  double layout_width  = max_x - min_x;
  double layout_height = max_y - min_y;

  double margin_y    = layout_height * 0.05;
  double core_min_y  = min_y + margin_y;
  double core_max_y  = max_y - margin_y;
  double core_height = core_max_y - core_min_y;

  int    internal_levels = num_levels - 2;
  double step_x          = layout_width / (internal_levels + 1);

  std::cout << "[Info] Dispensing INTERNAL cells only. Layout bounds: ("
            << min_x << ", " << min_y << ") to (" << max_x << ", " << max_y
            << ")\n";

  for (int l = 1; l < num_levels - 1; ++l) {
    int num_cells = levelized_cells[l].size();
    if (num_cells == 0)
      continue;

    double target_center_x = min_x + l * step_x;

    double step_y = core_height / (num_cells + 1);

    for (int i = 0; i < num_cells; ++i) {
      auto& cell = levelized_cells[l][i];

      double cell_w = 0.0;  
      double cell_h = 0.0;  

      double final_x = target_center_x - (cell_w / 2.0);

      double target_center_y = core_max_y - (i + 1) * step_y;
      double final_y         = target_center_y - (cell_h / 2.0);

      cell->setPosition(Point(final_x, final_y));
    }
  }

  std::cout << "[Info] Internal cells dispersed successfully. Input/Output "
               "positions preserved.\n";
}


std::vector<std::vector<std::shared_ptr<Pin>>> buildGraphAndFindPaths(
    const std::shared_ptr<Design>&                   design,
    std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    std::unordered_map<std::shared_ptr<Cell>, std::pair<double, double>>&
        neato_coords)
{
  neato_coords.clear();
  levelized_cells.clear();
  std::vector<std::shared_ptr<Cell>>        startpoints;
  std::unordered_set<std::shared_ptr<Cell>> endpoints;

  std::unordered_map<std::shared_ptr<Pin>, std::vector<std::shared_ptr<Pin>>>
                                                         graph;
  std::unordered_map<std::shared_ptr<Pin>, PinDirection> pin_dirs;
  std::unordered_set<std::shared_ptr<Net>>               visited_nets;

  std::unordered_map<std::shared_ptr<Cell>,
                     std::unordered_set<std::shared_ptr<Cell>>>
                                                 raw_cell_graph;
  std::unordered_map<std::shared_ptr<Cell>, int> raw_cell_in_degree;
  std::vector<std::vector<std::shared_ptr<Pin>>> all_paths;

  for (const auto& [inst_name, inst] : design->getCells()) {
    raw_cell_in_degree[inst] = 0;

    if (inst_name.find("gc") != std::string::npos) {
      if (inst_name.find("in") != std::string::npos) {
        startpoints.push_back(inst);
      } else {
        endpoints.insert(inst);
      }
    }
  }

  std::cout << "[Info] Building Pin Graph & Cell Graph (Full Design)..."
            << std::endl;
  for (const auto& [inst_name, inst] : design->getCells()) {
    for (const auto& [pin_name, pin] : inst->getPins()) {
      auto net = pin->getHostNet();
      if (net == nullptr)
        continue;

      if (visited_nets.find(net) == visited_nets.end()) {
        visited_nets.insert(net);
        auto net_pins = net->getPins();

        if (net_pins.size() >= 2) {
          auto p0          = net_pins[0];
          pin_dirs[p0]     = PinDirection::OUTPUT;
          auto driver_cell = p0->getHostCell();

          for (size_t i = 1; i < net_pins.size(); ++i) {
            auto p1      = net_pins[i];
            pin_dirs[p1] = PinDirection::INPUT;

            graph[p0].push_back(p1);

            auto recv_cell = p1->getHostCell();
            if (driver_cell != recv_cell) {
              if (raw_cell_graph[driver_cell].insert(recv_cell).second) {
                raw_cell_in_degree[recv_cell]++;
              }
            }
          }
        }
      }
    }
  }

  std::queue<std::shared_ptr<Cell>> topo_q;
  for (const auto& [cell, degree] : raw_cell_in_degree) {
    if (degree == 0)
      topo_q.push(cell);
  }

  levelized_cells.clear();
  size_t sorted_count = 0;

  std::cout << "[Info] Attempting standard Levelization..." << std::endl;
  while (!topo_q.empty()) {
    int                                level_size = topo_q.size();
    std::vector<std::shared_ptr<Cell>> current_level;

    for (int i = 0; i < level_size; ++i) {
      auto curr = topo_q.front();
      topo_q.pop();
      current_level.push_back(curr);
      sorted_count++;

      for (const auto& next_cell : raw_cell_graph[curr]) {
        if (--raw_cell_in_degree[next_cell] == 0) {
          topo_q.push(next_cell);
        }
      }
    }
    levelized_cells.push_back(current_level);
  }

  std::cout << "[Info] Adjusting startpoints to first level and endpoints to "
               "last level..."
            << std::endl;
  std::vector<std::vector<std::shared_ptr<Cell>>> adjusted_levels;
  std::unordered_set<std::shared_ptr<Cell>>       start_set(startpoints.begin(),
                                                      startpoints.end());

  if (!startpoints.empty()) {
    adjusted_levels.push_back(startpoints);
  }

  for (const auto& level : levelized_cells) {
    std::vector<std::shared_ptr<Cell>> mid_level;
    for (const auto& cell : level) {
      if (start_set.find(cell) == start_set.end()
          && endpoints.find(cell) == endpoints.end()) {
        mid_level.push_back(cell);
      }
    }
    if (!mid_level.empty()) {
      adjusted_levels.push_back(mid_level);
    }
  }

  if (!endpoints.empty()) {
    std::vector<std::shared_ptr<Cell>> end_level(endpoints.begin(),
                                                 endpoints.end());
    adjusted_levels.push_back(end_level);
  }

  levelized_cells = std::move(adjusted_levels);

  if (sorted_count < design->getCells().size()) {
    std::cerr << "[Warning] Combinational loop detected! Standard Levelization "
                 "failed.\n";
    std::cout << "[Info] Falling back to Graphviz neato (Force-Directed) "
                 "algorithm...\n";

    levelized_cells.clear();

    GVC_t*    gvc = gvContext();
    Agraph_t* g   = agopen((char*) "EDA_Force_Graph", Agdirected, nullptr);

    agsafeset(g, (char*) "rankdir", (char*) "LR", (char*) "");
    agattr(g, AGNODE, (char*) "style", (char*) "filled");
    agattr(g, AGNODE, (char*) "fillcolor", (char*) "lightblue");
    agattr(g, AGNODE, (char*) "fontname", (char*) "Helvetica");
    agattr(g, AGNODE, (char*) "label", (char*) "");
    agattr(g, AGNODE, (char*) "shape", (char*) "circle");
    agattr(g, AGRAPH, (char*) "mode", (char*) "KK");
    agattr(g, AGRAPH, (char*) "notranslate", (char*) "true");
    agattr(g, AGNODE, (char*) "pos", (char*) "");
    agattr(g, AGNODE, (char*) "pin", (char*) "");
    std::unordered_map<std::shared_ptr<Cell>, Agnode_t*> node_map;

    for (const auto& [inst_name, cell] : design->getCells()) {
      Agnode_t* n    = agnode(g, const_cast<char*>(cell->getName().c_str()), 1);
      node_map[cell] = n;
    }

    double layout_width  = 1200.0;
    double layout_height = 800.0;
    double offset_x = 0.0, offset_y = 0.0;
    double scale_factor = 0.001;

    auto design_shape = design->getShape();
    if (design_shape != nullptr) {
      auto points = design_shape->getPoints();
      if (!points.empty()) {
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();

        for (const auto& pt : points) {
          if (pt.x() < min_x)
            min_x = pt.x();
          if (pt.x() > max_x)
            max_x = pt.x();
          if (pt.y() < min_y)
            min_y = pt.y();
          if (pt.y() > max_y)
            max_y = pt.y();
        }
        offset_x      = min_x;
        offset_y      = min_y;
        layout_width  = (max_x - min_x) * scale_factor;
        layout_height = (max_y - min_y) * scale_factor;
      }
    }

    for (const auto& cell : startpoints) {
      Agnode_t* n = node_map[cell];
      if (n != nullptr) {
        auto   pos      = cell->getPosition();
        double scaled_x = (pos.x() - offset_x) * scale_factor;
        double scaled_y = (pos.y() - offset_y) * scale_factor;

        std::string pos_str
            = std::to_string(scaled_x) + "," + std::to_string(scaled_y) + "!";
        agset(n, (char*) "pos", const_cast<char*>(pos_str.c_str()));
        agset(n, (char*) "pin", (char*) "true");
      }
    }

    for (const auto& cell : endpoints) {
      Agnode_t* n = node_map[cell];
      if (n != nullptr) {
        auto   pos      = cell->getPosition();
        double scaled_x = (pos.x() - offset_x) * scale_factor;
        double scaled_y = (pos.y() - offset_y) * scale_factor;

        std::string pos_str
            = std::to_string(scaled_x) + "," + std::to_string(scaled_y) + "!";
        agset(n, (char*) "pos", const_cast<char*>(pos_str.c_str()));
        agset(n, (char*) "pin", (char*) "true");
      }
    }

    for (const auto& [u, targets] : raw_cell_graph) {
      for (const auto& v : targets) {
        agedge(g, node_map[u], node_map[v], nullptr, 1);
      }
    }

    gvLayout(gvc, g, "neato");

    // gvRenderFilename(gvc, g, "png", "debug_neato_fixed_layout.png");
    // gvRenderFilename(gvc, g, "dot", "debug_neato_fixed_layout.dot");

    for (const auto& [inst_name, cell] : design->getCells()) {
      Agnode_t* n       = node_map[cell];
      char*     pos_str = agget(n, (char*) "pos");
      double    x = 0.0, y = 0.0;
      if (pos_str != nullptr) {
        sscanf(pos_str, "%lf,%lf", &x, &y);
      }
      double real_x = (x / scale_factor) + offset_x;
      double real_y = (y / scale_factor) + offset_y;

      neato_coords[cell] = {real_x, real_y};
      cell->setPosition(Point(real_x, real_y));
    }

    gvFreeLayout(gvc, g);
    agclose(g);
    gvFreeContext(gvc);

    std::cout << "[Info] Neato resolution complete.\n";
    // TODO: Calc datapath, legalize neato coords
    return all_paths;
  } else {
    // TODO: Calc position crossing minimization
    minimizeCrossingsCP(design, levelized_cells, pin_dirs, 1.0, 10);
    std::cout << "[Info] Successfully minimize crossings" << std::endl;
    // dumpLevelizedCellsToDot(levelized_cells, pin_dirs);
    disperseInternalCellsTopologically(design, levelized_cells);
    std::cout << "[Info] Standard Levelization successful (" << sorted_count
              << " nodes).\n";
  }

  std::cout << "[Info] Searching data paths..." << std::endl;

  for (const auto& [inst_name, inst] : design->getCells()) {
    std::vector<std::shared_ptr<Pin>> inputs;
    std::vector<std::shared_ptr<Pin>> outputs;

    for (const auto& [pin_name, pin] : inst->getPins()) {
      if (pin_dirs[pin] == PinDirection::INPUT) {
        inputs.push_back(pin);
      } else if (pin_dirs[pin] == PinDirection::OUTPUT) {
        outputs.push_back(pin);
      }
    }

    for (const auto& in_pin : inputs) {
      for (const auto& out_pin : outputs) {
        graph[in_pin].push_back(out_pin);
      }
    }
  }

  auto dfs = [&](auto&                              self,
                 std::shared_ptr<Pin>               current_pin,
                 std::vector<std::shared_ptr<Pin>>& current_path) -> void {
    current_path.push_back(current_pin);
    auto current_cell = current_pin->getHostCell();

    if (endpoints.find(current_cell) != endpoints.end()
        && pin_dirs[current_pin] == PinDirection::INPUT) {
      all_paths.push_back(current_path);
    } else {
      for (const auto& next_pin : graph[current_pin]) {
        if (std::find(current_path.begin(), current_path.end(), next_pin)
            == current_path.end()) {
          self(self, next_pin, current_path);
        }
      }
    }
    current_path.pop_back();
  };

  for (const auto& start_cell : startpoints) {
    for (const auto& [pin_name, pin] : start_cell->getPins()) {
      if (pin_dirs[pin] == PinDirection::OUTPUT) {
        std::vector<std::shared_ptr<Pin>> current_path;
        dfs(dfs, pin, current_path);
      }
    }
  }

  std::cout << "[Info] Found " << all_paths.size() << " data paths.\n";
  return all_paths;
}

void GlobalRouter::incrementalPlace(double region_margin,
                                    double bend_radius,
                                    int    K)
{
  bool improvement = true;
  while (improvement) {
    improvement         = false;
    auto critical_cells = ilg_->getCritCells();
    int  count          = 0;

    for (auto critical_cell : critical_cells) {
      count++;
      std::cout << "count: " << count << "/" << critical_cells.size()
                << std::endl;

      auto targetRegion = getTargetRegion(critical_cell.first, region_margin);
      auto subRegions = getSubRegions(critical_cell, targetRegion, bend_radius);
      sortSubRegions(subRegions);
      auto medianPoint = getMedian(critical_cell);
      std::vector<std::pair<Point, std::vector<int>>> X;

      for (auto subRegion : subRegions) {
        Point x1;
        if (isInRegion(medianPoint, subRegion.first)) {
          x1 = medianPoint;
        } else {
          x1 = projectToRegion(medianPoint, subRegion.first);
        }
        x1 = Point(std::round(x1.x() * 1000.0) / 1000.0,
                   std::round(x1.y() * 1000.0) / 1000.0);

        if (X.size() >= K) {
          break;
        } else {
          X.push_back({x1, subRegion.second});
        }
      }

      auto             routingInfo = getRoutingInfo();
      auto             bestX       = critical_cell.first->getPosition();
      auto             bestIL      = ilg_->getMaxIL();
      std::vector<int> bestWeight;
      for (auto x2 : X) {
        critical_cell.first->setPosition(x2.first);
        if (isBlocked(critical_cell.first)) {
          critical_cell.first->setPosition(bestX);
          continue;
        }
        bool isSuccess = rerouteCellNets(critical_cell.first);
        auto newIL     = ilg_->getMaxIL();
        if (newIL < bestIL && isSuccess) {
          bestIL     = newIL;
          bestX      = x2.first;
          bestWeight = x2.second;
          critical_cell.first->setPosition(x2.first);
          routingInfo = getRoutingInfo();
          improvement = true;
        } else {
          critical_cell.first->setPosition(bestX);
          setRoutingInfo(routingInfo);
        }
      }
      if (improvement) {
        std::cout << "==============================" << std::endl;
        std::cout << "bestIL: " << bestIL << std::endl;
        std::cout << "X: ";
        for (auto x3 : X) {
          std::cout << "(" << x3.first.x() << ", " << x3.first.y() << ")"
                    << x3.second[0] << " " << x3.second[1] << "      ";
        }
        std::cout << std::endl;
        std::cout << "Critical Cell: ";
        for (auto cell : critical_cells) {
          std::cout << cell.first->getName() << " ";
        }
        std::cout << std::endl;
        std::cout << "[Placement] Moved cell " << critical_cell.first->getName()
                  << " to (" << bestX.x() << ", " << bestX.y()
                  << ") with weight [";
        for (size_t i = 0; i < bestWeight.size(); ++i) {
          std::cout << bestWeight[i] << " ";
        }
        std::cout << "]" << std::endl;
        // exportToSVG(critical_cell.first->getName() + "["
        //                 + std::to_string(bestWeight[0]) + ","
        //                 + std::to_string(bestWeight[1]) + "].svg",
        //             critical_cells,
        //             targetRegion,
        //             subRegions);
        // auto pins
        //     = critical_cell.first->getPins();  // Copy the map (or use const
        //                                        // auto& if it returns a ref)
        // for (auto& pins_entry : pins) {
        //   auto& pin = pins_entry.second;
        //   auto  net = pin->getHostNet();  // Copy the shared_ptr
        //   if (net) {
        //     std::cout << net->printPins() << std::endl;
        //   }
        // }
        break;
      }
    }
  }
}