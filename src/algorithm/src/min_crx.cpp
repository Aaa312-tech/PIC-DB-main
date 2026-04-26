#include "min_crx.h"

using namespace operations_research;
using namespace operations_research::sat;

int get_port_offset(const std::shared_ptr<Pin>& target_pin, 
                    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs) {
    if (!target_pin) return 0;

    // SAFE CHECK: Ensure the target pin itself is mapped
    auto target_it = pin_dirs.find(target_pin);
    if (target_it == pin_dirs.end()) return 0; 
    
    PinDirection target_dir = target_it->second;

    auto cell = target_pin->getHostCell();
    if (!cell) return 0;

    std::vector<std::shared_ptr<Pin>> same_side_pins;

    for (const auto& [pin_name, pin] : cell->getPins()) {
        // SAFE CHECK: Use find() instead of at() to ignore unmapped/floating pins
        auto it = pin_dirs.find(pin);
        if (it != pin_dirs.end() && it->second == target_dir) {
            same_side_pins.push_back(pin);
        }
    }

    std::sort(same_side_pins.begin(), same_side_pins.end(),
        [](const std::shared_ptr<Pin>& a, const std::shared_ptr<Pin>& b) {
            return a->getPosition().y() > b->getPosition().y(); 
        }
    );

    auto it = std::find(same_side_pins.begin(), same_side_pins.end(), target_pin);
    if (it != same_side_pins.end()) {
        return std::distance(same_side_pins.begin(), it);
    }

    return 0;
}

void minimizeCrossingsCP(
    const std::shared_ptr<Design>& design,
    std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs,
    double lambda_worst_path,
    int max_ports
) {
    int K_layers = levelized_cells.size();
    if (K_layers < 2) return;

    auto sort_by_physical_y = [](const std::shared_ptr<Cell>& a, const std::shared_ptr<Cell>& b) {
        return a->getPosition().y() > b->getPosition().y(); 
    };

    if (levelized_cells[0].size() > 1) {
        std::sort(levelized_cells[0].begin(), levelized_cells[0].end(), sort_by_physical_y);
    }
    if (levelized_cells[K_layers - 1].size() > 1) {
        std::sort(levelized_cells[K_layers - 1].begin(), levelized_cells[K_layers - 1].end(), sort_by_physical_y);
    }

    std::vector<std::vector<std::shared_ptr<OptNode>>> opt_layers(K_layers);
    std::unordered_map<std::shared_ptr<Cell>, std::shared_ptr<OptNode>> cell_to_opt;
    int next_node_id = 0;

    for (int l = 0; l < K_layers; ++l) {
        for (const auto& cell : levelized_cells[l]) {
            auto node = std::make_shared<OptNode>();
            node->is_dummy = false;
            node->real_cell = cell;
            node->layer_idx = l;
            node->id = next_node_id++;
            opt_layers[l].push_back(node);
            cell_to_opt[cell] = node;
        }
    }

    std::vector<std::vector<OptWire>> E_l(K_layers - 1); 
    std::unordered_set<std::shared_ptr<Net>> visited_nets;
    
    for (const auto& [inst_name, inst] : design->getCells()) {
        for (const auto& [pin_name, pin] : inst->getPins()) {
            auto net = pin->getHostNet();
            if (!net || visited_nets.count(net)) continue;
            visited_nets.insert(net);

            auto net_pins = net->getPins();
            std::shared_ptr<Pin> driver_pin = nullptr;
            std::vector<std::shared_ptr<Pin>> recv_pins;

            for (const auto& p : net_pins) {
                auto it = pin_dirs.find(p);
                if (it != pin_dirs.end()) {
                    if (it->second == PinDirection::OUTPUT) {
                        driver_pin = p;
                    } else if (it->second == PinDirection::INPUT) {
                        recv_pins.push_back(p);
                    }
                }
            }

            if (!driver_pin) continue;
            auto u_cell = driver_pin->getHostCell();
            if (cell_to_opt.find(u_cell) == cell_to_opt.end()) continue;
            
            auto u_node = cell_to_opt[u_cell];
            int lu = u_node->layer_idx;
            int pu = get_port_offset(driver_pin, pin_dirs);

            for (const auto& v_pin : recv_pins) {
                auto v_cell = v_pin->getHostCell();
                if (u_cell == v_cell || cell_to_opt.find(v_cell) == cell_to_opt.end()) continue;
                
                auto v_node = cell_to_opt[v_cell];
                int lv = v_node->layer_idx;
                int pv = get_port_offset(v_pin, pin_dirs);

                if (lu >= lv) continue; 

                if (lv - lu == 1) {
                    E_l[lu].push_back({u_node, pu, v_node, pv});
                } else {
                    std::shared_ptr<OptNode> prev_node = u_node;
                    int prev_port = pu;

                    for (int mid_layer = lu + 1; mid_layer < lv; ++mid_layer) {
                        auto dummy = std::make_shared<OptNode>();
                        dummy->is_dummy = true;
                        dummy->real_cell = nullptr;
                        dummy->layer_idx = mid_layer;
                        dummy->id = next_node_id++;
                        
                        opt_layers[mid_layer].push_back(dummy);
                        E_l[mid_layer - 1].push_back({prev_node, prev_port, dummy, 0});
                        
                        prev_node = dummy;
                        prev_port = 0; 
                    }
                    E_l[lv - 1].push_back({prev_node, prev_port, v_node, pv});
                }
            }
        }
    }

    CpModelBuilder cp_model;

    const int K_grid = max_ports + 1; 
    const int max_cost_upper_bound = 100000; 

    std::unordered_map<int, IntVar> r_var;     
    std::unordered_map<int, IntVar> d_var;     
    std::map<std::pair<int, int>, IntVar> wload_var; 
    std::vector<IntVar> all_wload_vars;

    for (int l = 0; l < K_layers; ++l) {
        int n = opt_layers[l].size();
        bool is_fixed_layer = (l == 0 || l == K_layers - 1);
        std::vector<IntVar> layer_r_vars;

        for (int i = 0; i < n; ++i) {
            auto node = opt_layers[l][i];
            
            d_var[node->id] = cp_model.NewIntVar({0, max_cost_upper_bound});

            if (is_fixed_layer) {
                r_var[node->id] = cp_model.NewConstant(i);
            } else {
                r_var[node->id] = cp_model.NewIntVar({0, n - 1});
            }
            layer_r_vars.push_back(r_var[node->id]);
        }

        if (!is_fixed_layer && layer_r_vars.size() > 1) {
            cp_model.AddAllDifferent(layer_r_vars);
        }
    }

    for (const auto& node : opt_layers[0]) {
        cp_model.AddEquality(d_var[node->id], 0);
    }

    for (int l = 0; l < K_layers - 1; ++l) {
        int nW = E_l[l].size();
        std::vector<std::vector<BoolVar>> edge_crossings(nW);

        for (int i = 0; i < nW; ++i) {
            wload_var[{l, i}] = cp_model.NewIntVar({0, nW});
            all_wload_vars.push_back(wload_var[{l, i}]);
            
            const auto& w = E_l[l][i];
            cp_model.AddGreaterOrEqual(d_var[w.dst->id], d_var[w.src->id] + wload_var[{l, i}]);
        }

        for (int i = 0; i < nW; ++i) {
            for (int j = i + 1; j < nW; ++j) {
                const auto& w1 = E_l[l][i];
                const auto& w2 = E_l[l][j];

                if ((w1.src == w2.src && w1.src_port == w2.src_port) ||
                    (w1.dst == w2.dst && w1.dst_port == w2.dst_port)) {
                    continue;
                }

                LinearExpr y_src1 = r_var[w1.src->id] * K_grid + w1.src_port;
                LinearExpr y_src2 = r_var[w2.src->id] * K_grid + w2.src_port;
                LinearExpr y_dst1 = r_var[w1.dst->id] * K_grid + w1.dst_port;
                LinearExpr y_dst2 = r_var[w2.dst->id] * K_grid + w2.dst_port;

                BoolVar b_src = cp_model.NewBoolVar(); 
                BoolVar b_dst = cp_model.NewBoolVar(); 
                BoolVar chi   = cp_model.NewBoolVar(); 

                cp_model.AddGreaterOrEqual(y_src1, y_src2 + 1).OnlyEnforceIf(b_src);
                cp_model.AddLessOrEqual(y_src1, y_src2 - 1).OnlyEnforceIf(b_src.Not());
                
                cp_model.AddGreaterOrEqual(y_dst1, y_dst2 + 1).OnlyEnforceIf(b_dst);
                cp_model.AddLessOrEqual(y_dst1, y_dst2 - 1).OnlyEnforceIf(b_dst.Not());

                cp_model.AddNotEqual(b_src, b_dst).OnlyEnforceIf(chi);  
                cp_model.AddEquality(b_src, b_dst).OnlyEnforceIf(chi.Not()); 

                edge_crossings[i].push_back(chi);
                edge_crossings[j].push_back(chi);
            }
        }

        for (int i = 0; i < nW; ++i) {
            if (!edge_crossings[i].empty()) {
                cp_model.AddEquality(wload_var[{l, i}], LinearExpr::Sum(edge_crossings[i]));
            } else {
                cp_model.AddEquality(wload_var[{l, i}], 0);
            }
        }
    }

    IntVar max_c = cp_model.NewIntVar({0, max_cost_upper_bound});
    std::vector<IntVar> final_layer_costs;
    for (const auto& node : opt_layers.back()) {
        final_layer_costs.push_back(d_var[node->id]);
    }
    
    if (!final_layer_costs.empty()) {
        cp_model.AddMaxEquality(max_c, final_layer_costs);
    } else {
        cp_model.AddEquality(max_c, 0);
    }

    const int SCALE = 1;
    int lambda_int = std::round(lambda_worst_path * SCALE);
    
    if (all_wload_vars.empty()) {
        cp_model.Minimize(max_c * lambda_int);
    } else {
        LinearExpr total_w = LinearExpr::Sum(all_wload_vars);
        cp_model.Minimize(max_c * lambda_int + total_w * SCALE);
    }

    std::cout << "[Info] CP-SAT Model Built. Variables: " << cp_model.Build().variables_size() << "\n";
    Model model;
    
    SatParameters parameters;
    parameters.set_max_time_in_seconds(60.0); 
    parameters.set_num_search_workers(8); 
    model.Add(NewSatParameters(parameters));

    const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);

    if (response.status() == CpSolverStatus::OPTIMAL || response.status() == CpSolverStatus::FEASIBLE) {
        std::cout << "[Info] Optimization Success! Status: " 
                  << (response.status() == CpSolverStatus::OPTIMAL ? "OPTIMAL" : "FEASIBLE") 
                  << ". Objective: " << response.objective_value() / static_cast<double>(SCALE) << "\n";

        for (int l = 0; l < K_layers; ++l) {
            std::vector<std::pair<std::shared_ptr<Cell>, int>> scored_cells;
            for (const auto& node : opt_layers[l]) {
                if (node->is_dummy) continue;
                int rank_val = SolutionIntegerValue(response, r_var[node->id]);
                scored_cells.push_back({node->real_cell, rank_val});
                
                std::cout << "  Layer " << l << " | Cell: " << node->real_cell->getName() 
                          << " -> Assigned Rank: " << rank_val << "\n";
            }

            std::sort(scored_cells.begin(), scored_cells.end(), 
                      [](const auto& a, const auto& b) { return a.second < b.second; });

            levelized_cells[l].clear();
            for (auto& pair : scored_cells) {
                levelized_cells[l].push_back(pair.first);
            }
        }
    } else {
        std::cerr << "[Warning] CP-SAT Failed to find a feasible solution.\n";
    }
}