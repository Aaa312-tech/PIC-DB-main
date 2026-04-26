#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "design.h"
#include "min_crx.h"
#include "ortools/sat/cp_model.h"
#include "route_config.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

#ifdef __cplusplus
}
#endif

void printGraphDebugInfo(
    const std::vector<std::shared_ptr<Cell>>&                     startpoints,
    const std::unordered_set<std::shared_ptr<Cell>>&              endpoints,
    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs,
    const std::unordered_map<std::shared_ptr<Pin>,
                             std::vector<std::shared_ptr<Pin>>>&  graph);

void dumpLevelizedCellsToDot(
    const std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    const std::unordered_map<std::shared_ptr<Pin>, PinDirection>& pin_dirs,
    const std::string& filename = "levelized_graph.dot");

void disperseInternalCellsTopologically(
    const std::shared_ptr<Design>&                         design,
    const std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells);


std::vector<std::vector<std::shared_ptr<Pin>>> buildGraphAndFindPaths(
    const std::shared_ptr<Design>&                   design,
    std::vector<std::vector<std::shared_ptr<Cell>>>& levelized_cells,
    std::unordered_map<std::shared_ptr<Cell>, std::pair<double, double>>&
        neato_coords);