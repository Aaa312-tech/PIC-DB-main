#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// forward declarations (project types)
class Database;
class Design;
class Cell;
class Pin;
struct PlaceConfig;

class PlaceManager
{
 public:
  struct Setting
  {
    // CG
    int    max_cg_iterations = 300;
    double cg_grad_rel_tol   = 1e-6;
    double cg_energy_rel_tol = 1e-4;
    int    cg_stall_iters    = 10;

    // Line search
    double armijo_c1          = 1e-3;
    double line_search_alpha0 = 100.0;
    double line_search_tau    = 0.5;

    // Density grid
    int    bin_count_x    = 120;
    int    bin_count_y    = 80;
    double lambda_density = 0.1;

    // Wirelength
    double lse_alpha = 400.0;

    // Poisson
    int    poisson_max_iter = 300;
    double poisson_tol      = 1e-3;

    // ----------------------------
    // Congestion (RUDY)
    // ----------------------------
    int    cong_bin_count_x  = 1000;
    int    cong_bin_count_y  = 1000;
    double lambda_congestion = 0.0;  // 0 => off
    double cong_target_util  = 0.8;
    double cong_softplus_k   = 10.0;
    double cong_cap_h        = 1.0;
    double cong_cap_v        = 1.0;

    // Switch
    bool consider_orientation = false;
    bool consider_data_path   = true;
    bool center_init          = true;
    bool bending_penalty      = true;
    bool output_yml           = false;
  };

  PlaceManager(std::shared_ptr<Database> database, const PlaceConfig& cfg);

  void placeAll();
  void centerInit();


  struct Size
  {
    double w;
    double h;
  };

  struct PinOffset
  {
    std::shared_ptr<Pin> pin;
    double               xor_rate;
    double               yor_rate;
  };

  struct PlacementVariables
  {
    std::vector<std::shared_ptr<Cell>>   movable_cells;
    std::unordered_map<std::string, int> cell_index;
    std::vector<Size>                    base_size;
    std::vector<std::vector<PinOffset>>  pin_offsets;
  };

 private:
  struct ElectrostaticGrid
  {
    int    nx, ny;
    double xmin, xmax, ymin, ymax;
    double binW, binH;
    double binArea;

    std::vector<std::vector<double>> density;
    std::vector<std::vector<double>> charge;
    std::vector<std::vector<double>> potential;
    std::vector<std::vector<double>> Ex;
    std::vector<std::vector<double>> Ey;
  };

  Size   getSize(const std::shared_ptr<Cell>& cell) const;
  double getWireLength() const;

  PlacementVariables buildPlacementVariables();

  void addEdge(const std::string&                                         u,
               const std::string&                                         v,
               std::unordered_map<std::string, std::vector<std::string>>& adj,
               std::unordered_map<std::string, int>&                      indeg,
               std::unordered_set<std::string>& nodes);

  std::vector<std::vector<std::string>> topoSortLevels(
      const std::unordered_map<std::string, std::vector<std::string>>& adj,
      std::unordered_map<std::string, int>                             indeg,
      const std::unordered_set<std::string>&                           nodes);

  std::vector<std::vector<std::string>> regularityExtraction();

  std::vector<double> extractPositionVectorXY();
  std::vector<double> extractPositionVectorXYZ();

  void applyPositionVectorXY(std::vector<double>& x);
  void applyPositionVectorXYZ(std::vector<double>& x);

  ElectrostaticGrid buildElectrostaticGrid();
  double            solvePoissonAndField(ElectrostaticGrid& g);

  double computeLSEWirelengthAndGradientXY(std::vector<double>& grad);
  double computeLSEWirelengthAndGradientXYZ(std::vector<double>& grad);

  double computeEnergyAndGradientXY(std::vector<double>& grad);
  double computeEnergyAndGradientXYZ(std::vector<double>& grad);

  double dotProduct(const std::vector<double>& a, const std::vector<double>& b);

  double vectorNorm(const std::vector<double>& a);

  void axpy(std::vector<double>& y, double alpha, const std::vector<double>& x);

  void dataPathConstraint(std::vector<double>& x, int iteration, int max_iter);

  void nonlinearConjugateGradient(
      std::vector<double>&                             x,
      const std::function<void(std::vector<double>&)>& applyFn,
      const std::function<double(std::vector<double>&, std::vector<double>&)>&
          energyFn);

 private:
  std::shared_ptr<Database> _database;
  std::shared_ptr<Design>   _design;
  Setting                   setting;

  PlacementVariables                    vars;
  std::vector<std::vector<std::string>> levels;
};