#pragma once
#include <exception>
#include <string>
#include <vector>

#include "log.h"
#include "yml.h"

struct PlaceStageConfig
{
  // -------- CG --------
  int    max_cg_iterations = 300;
  double cg_grad_rel_tol   = 1e-6;
  double cg_energy_rel_tol = 1e-4;
  int    cg_stall_iters    = 10;

  // -------- Line search --------
  double armijo_c1          = 1e-3;
  double line_search_alpha0 = 100.0;
  double line_search_tau    = 0.5;

  // -------- Density / Grid --------
  int    bin_count_x    = 120;
  int    bin_count_y    = 80;
  double lambda_density = 0.1;

  // -------- Wirelength --------
  double lse_alpha = 400.0;

  // -------- Poisson --------
  int    poisson_max_iter = 300;
  double poisson_tol      = 1e-3;

  // -------- Congestion (RUDY) --------
  int    cong_bin_count_x  = 1000;
  int    cong_bin_count_y  = 1000;
  double lambda_congestion = 0.0;  // default off
  double cong_target_util  = 0.8;
  double cong_softplus_k   = 10.0;
  double cong_cap_h        = 1.0;
  double cong_cap_v        = 1.0;
};

struct PlaceConfig
{
  // -------- Multi-stage global placement --------
  std::vector<PlaceStageConfig> stages;

  // -------- Switches (global) --------
  bool consider_orientation = false;
  bool consider_data_path   = true;
  bool center_init          = true;
  bool bending_penalty      = true;
  bool output_yml           = false;

  // -------- Backward compatibility (optional) --------
  PlaceStageConfig single_stage;

  // API
  static PlaceConfig loadFromYAML(const std::string& path)
  {
    PlaceConfig cfg;

    try {
      Yml yml(path);

      // =========================================================
      // 1) Global switches (always top-level)
      // =========================================================
      if (yml.exists("switch")) {
        auto n = yml.getNode("switch");
        Yml::read(n, "consider_orientation", cfg.consider_orientation);
        Yml::read(n, "consider_data_path", cfg.consider_data_path);
        Yml::read(n, "center_init", cfg.center_init);
        Yml::read(n, "bending_penalty", cfg.bending_penalty);
        Yml::read(n, "output_yml", cfg.output_yml);
      }

      // =========================================================
      // 2) Multi-stage global placement
      // =========================================================
      if (yml.exists("global_place") && yml.exists("global_place.stages")) {
        auto stagesNode = yml.getNode("global_place.stages");

        if (!stagesNode.IsSequence()) {
          throw std::runtime_error("global_place.stages must be a sequence");
        }

        for (const auto& s : stagesNode) {
          PlaceStageConfig stage;
          loadStageConfig(s, stage);
          cfg.stages.push_back(stage);
        }

        if (cfg.stages.empty()) {
          throw std::runtime_error("global_place.stages is empty");
        }
      }

      // =========================================================
      // 3) Legacy single-stage fallback
      // =========================================================
      if (cfg.stages.empty()) {
        PlaceStageConfig& stage = cfg.single_stage;

        if (yml.exists("cg")) {
          auto n = yml.getNode("cg");
          Yml::read(n, "max_iterations", stage.max_cg_iterations);
          Yml::read(n, "grad_rel_tol", stage.cg_grad_rel_tol);
          Yml::read(n, "energy_rel_tol", stage.cg_energy_rel_tol);
          Yml::read(n, "stall_iters", stage.cg_stall_iters);
        }

        if (yml.exists("line_search")) {
          auto n = yml.getNode("line_search");
          Yml::read(n, "armijo_c1", stage.armijo_c1);
          Yml::read(n, "alpha0", stage.line_search_alpha0);
          Yml::read(n, "tau", stage.line_search_tau);
        }

        if (yml.exists("grid")) {
          auto n = yml.getNode("grid");
          Yml::read(n, "bin_count_x", stage.bin_count_x);
          Yml::read(n, "bin_count_y", stage.bin_count_y);
        }

        if (yml.exists("density")) {
          auto n = yml.getNode("density");
          Yml::read(n, "lambda", stage.lambda_density);
        }

        if (yml.exists("wirelength")) {
          auto n = yml.getNode("wirelength");
          Yml::read(n, "lse_alpha", stage.lse_alpha);
        }

        if (yml.exists("poisson")) {
          auto n = yml.getNode("poisson");
          Yml::read(n, "max_iter", stage.poisson_max_iter);
          Yml::read(n, "tol", stage.poisson_tol);
        }

        // Congestion legacy block
        if (yml.exists("congestion")) {
          auto n = yml.getNode("congestion");
          Yml::read(n, "cong_bin_count_x", stage.cong_bin_count_x);
          Yml::read(n, "cong_bin_count_y", stage.cong_bin_count_y);
          Yml::read(n, "lambda", stage.lambda_congestion);
          Yml::read(n, "target_util", stage.cong_target_util);
          Yml::read(n, "softplus_k", stage.cong_softplus_k);
          Yml::read(n, "cap_h", stage.cong_cap_h);
          Yml::read(n, "cap_v", stage.cong_cap_v);
        }
      }

    } catch (const std::exception& e) {
      OPDB::logger->error(
          "[PlaceConfig] Failed to load {}: {}. Using default values.",
          path,
          e.what());
    }
    return cfg;
  }

 private:
  static void loadStageConfig(const YAML::Node& node, PlaceStageConfig& stage)
  {
    if (node["cg"]) {
      auto n = node["cg"];
      Yml::read(n, "max_iterations", stage.max_cg_iterations);
      Yml::read(n, "grad_rel_tol", stage.cg_grad_rel_tol);
      Yml::read(n, "energy_rel_tol", stage.cg_energy_rel_tol);
      Yml::read(n, "stall_iters", stage.cg_stall_iters);
    }

    if (node["line_search"]) {
      auto n = node["line_search"];
      Yml::read(n, "armijo_c1", stage.armijo_c1);
      Yml::read(n, "alpha0", stage.line_search_alpha0);
      Yml::read(n, "tau", stage.line_search_tau);
    }

    if (node["grid"]) {
      auto n = node["grid"];
      Yml::read(n, "bin_count_x", stage.bin_count_x);
      Yml::read(n, "bin_count_y", stage.bin_count_y);
    }

    if (node["density"]) {
      auto n = node["density"];
      Yml::read(n, "lambda", stage.lambda_density);
    }

    if (node["wirelength"]) {
      auto n = node["wirelength"];
      Yml::read(n, "lse_alpha", stage.lse_alpha);
    }

    if (node["poisson"]) {
      auto n = node["poisson"];
      Yml::read(n, "max_iter", stage.poisson_max_iter);
      Yml::read(n, "tol", stage.poisson_tol);
    }

    // Congestion per-stage block
    if (node["congestion"]) {
      auto n = node["congestion"];
      Yml::read(n, "lambda", stage.lambda_congestion);
      Yml::read(n, "target_util", stage.cong_target_util);
      Yml::read(n, "softplus_k", stage.cong_softplus_k);
      Yml::read(n, "cap_h", stage.cong_cap_h);
      Yml::read(n, "cap_v", stage.cong_cap_v);
    }
  }
};