#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <fstream>
#include "design.h"
#include "route_config.h"
#include "ortools/sat/cp_model.h"
#include "fast_route.h"

using namespace operations_research;
using namespace operations_research::sat;
struct OptNode {
    bool is_dummy;
    std::shared_ptr<Cell> real_cell;
    int layer_idx;
    int id; 
};

struct OptWire {
    std::shared_ptr<OptNode> src;
    int src_port;
    std::shared_ptr<OptNode> dst;
    int dst_port;
};

int get_port_offset(const std::shared_ptr<Pin>& target_pin, 
                    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs);

void minimizeCrossingsCP(
    const std::shared_ptr<Design>& design,
    std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs,
    double lambda_worst_path = 1.0,
    int max_ports = 10
);