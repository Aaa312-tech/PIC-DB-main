#pragma once
#include <exception>
#include <string>

#include "log.h"
#include "yml.h"

struct RouteConfig
{
  // Routing parameters
  int                                     width     = 5;
  int                                     spacing   = 5;
  double                                  loss_cr   = 0.52;
  double                                  loss_pp   = 1.5;
  double                                  loss_bend = 0.005;
  std::unordered_map<std::string, double> loss_comp;

  double incre_margin = 100.0;
  double incre_radius = 5.0;
  int    incre_K      = 10;
  // API
  static RouteConfig loadFromYAML(const std::string& path)
  {
    RouteConfig cfg;

    try {
      Yml yml(path);
      // Load routing parameters
      if (yml.exists("dr")) {
        auto n = yml.getNode("dr");
        Yml::read(n, "width", cfg.width);
        Yml::read(n, "spacing", cfg.spacing);
        Yml::read(n, "loss_cr", cfg.loss_cr);
        Yml::read(n, "loss_pp", cfg.loss_pp);
        Yml::read(n, "loss_bend", cfg.loss_bend);
        if (n["loss_comp"] && n["loss_comp"].IsMap()) {
          for (auto it = n["loss_comp"].begin(); it != n["loss_comp"].end();
               ++it) {
            std::string comp_name    = it->first.as<std::string>();
            double      comp_loss    = it->second.as<double>();
            cfg.loss_comp[comp_name] = comp_loss;
          }
        }
      }

      if (yml.exists("incrementalPlacement")) {
        auto n = yml.getNode("incrementalPlacement");
        Yml::read(n, "incre_margin", cfg.incre_margin);
        Yml::read(n, "incre_radius", cfg.incre_radius);
        Yml::read(n, "incre_K", cfg.incre_K);
      }

    } catch (const std::exception& e) {
      OPDB::logger->warn(
          "[RouteConfig] Failed to load {}: {}. Using default values.",
          path,
          e.what());
    }

    return cfg;
  }
};