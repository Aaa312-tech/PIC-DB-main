#pragma once

// C++ Standard Library
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <regex>
#include <map>
#include <cmath>

// Project Headers
#include "cell.h"
#include "design.h"
#include "log.h"
#include "net.h"
#include "pin.h"
#include "shape.h"


class LiDARYml
{
 public:
  LiDARYml() = default;

  static std::string fmt(double val)
  {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << val;
    return oss.str();
  }

  static void fromDesignToLiDARYml(const Design&      design,
                                   const std::string& liDARYmlOutPath)
  {
    YAML::Emitter out;

    out << YAML::BeginMap;
    // schema: null
    out << YAML::Key << "schema" << YAML::Value << YAML::Null;

    // ports: {}
    out << YAML::Key << "ports" << YAML::Value << YAML::BeginMap
        << YAML::EndMap;

    // schema_version: 1
    out << YAML::Key << "schema_version" << YAML::Value << 1;

    // settings:
    out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "version" << YAML::Value << YAML::DoubleQuoted << "1.0";
    out << YAML::Key << "design" << YAML::Value << design.getName();
    out << YAML::Key << "units_distance_microns" << YAML::Value << 1;

    std::vector<Point> points = design.getShape()->getPoints();
    out << YAML::Key << "die_area" << YAML::Value << YAML::BeginSeq;
    for (const auto& point : points) {
      out << YAML::BeginSeq << fmt(point.x()) << fmt(point.y()) << YAML::EndSeq;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "num_instances" << YAML::Value
        << design.getCells().size();
    out << YAML::Key << "num_nets" << YAML::Value << design.getNets().size();
    out << YAML::Key << "num_ports" << YAML::Value << 0;
    out << YAML::Key << "wg_radius" << YAML::Value << 5;
    out << YAML::EndMap;  // End settings map

    // instances:
    out << YAML::Key << "instances" << YAML::Value << YAML::BeginMap;
    for (const auto& cellPair : design.getCells()) {
      const std::string&           instanceName = cellPair.first;
      const std::shared_ptr<Cell>& cell         = cellPair.second;
      
      std::string originalType = cell->getType();
      std::string baseComponent = originalType;
      std::map<std::string, int> extractedSettings;
      bool isMmi = false; 
      bool isHeater = false;
      bool isStraight = false;

      if (baseComponent.rfind("m_", 0) == 0) { 
        if (baseComponent.find("mmi_I") != std::string::npos) {
          std::regex inRegex("_I(\\d+)");
          std::regex outRegex("_O(\\d+)");
          std::smatch inMatch, outMatch;
          
          if (std::regex_search(baseComponent, inMatch, inRegex) && 
              std::regex_search(baseComponent, outMatch, outRegex)) {
            extractedSettings["inputs"] = std::stoi(inMatch[1].str());
            extractedSettings["outputs"] = std::stoi(outMatch[1].str());
          }
          baseComponent = "mmi";
          isMmi = true; 
        } else if (baseComponent.find("mmi1x2") != std::string::npos) {
          baseComponent = "mmi1x2";
        } else if (baseComponent.find("mmi2x2") != std::string::npos) {
          baseComponent = "mmi2x2";
        } else if (baseComponent.find("mzi") != std::string::npos) {
          baseComponent = "mzi";
        } else if (baseComponent.find("straight_heater_metal") != std::string::npos) {
          if (baseComponent.find("_u_") != std::string::npos || baseComponent.find("undercut") != std::string::npos) {
            baseComponent = "straight_heater_metal_undercut";
          } else {
            baseComponent = "straight_heater_metal";
          }
          isHeater = true;
        }else if (baseComponent.find("straight") != std::string::npos && 
            baseComponent.find("heater") == std::string::npos) {
            baseComponent = "straight";
            isStraight = true;
        } 
        else if (baseComponent.find("grating_coupler") != std::string::npos) {
          baseComponent = "grating_coupler_elliptical_lumerical";
        }
      }

      auto rect  = std::dynamic_pointer_cast<Rect>(cell->getShape());
      double width_val = 0.0;
      double height_val = 0.0;
      if (rect) {
        width_val = rect->getShape().max_corner().x() - rect->getShape().min_corner().x();
        height_val = rect->getShape().max_corner().y() - rect->getShape().min_corner().y();
      }

      out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "component" << YAML::Value << baseComponent;
      
      out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;
      if (isMmi) {
        for (const auto& kv : extractedSettings) {
          out << YAML::Key << kv.first << YAML::Value << kv.second;
        }
        out << YAML::Key << "width" << YAML::Value << YAML::Null;
        out << YAML::Key << "input_positions" << YAML::Value << YAML::Null;
        out << YAML::Key << "output_positions" << YAML::Value << YAML::Null;
        out << YAML::Key << "length_mmi" << YAML::Value << width_val - 20;
        out << YAML::Key << "width_mmi" << YAML::Value << height_val;
        out << YAML::Key << "width_taper" << YAML::Value << 1.0;
        out << YAML::Key << "length_taper" << YAML::Value << 10.0;
        out << YAML::Key << "gap_input_tapers" << YAML::Value << 4;
        out << YAML::Key << "gap_output_tapers" << YAML::Value << 4;
        out << YAML::Key << "taper" << YAML::Value << "taper";
        out << YAML::Key << "straight" << YAML::Value << "straight";
        out << YAML::Key << "cross_section" << YAML::Value << "strip";
      }
      
      if (isHeater) {
        out << YAML::Key << "length" << YAML::Value << 200.0;
      }

      if(isStraight){
        out << YAML::Key << "length" << YAML::Value << 200.0;
      }

      out << YAML::Key << "macro_type" << YAML::Value << originalType; 
      

      out << YAML::Key << "placement" << YAML::Value << YAML::BeginSeq;

      std::string cellPlacement;
      switch (cell->getPlacement()) {
        case Placement::FIXED:
          cellPlacement = "FIXED";
          break;
        case Placement::PLACED:
          cellPlacement = "PLACED";
          break;
        default:
          cellPlacement = "UNPLACED";
          break;
      }
      out << cellPlacement;

      // auto rect  = std::dynamic_pointer_cast<Rect>(cell->getShape());
      auto width = rect->getShape().max_corner().x()
                   - rect->getShape().min_corner().x();
      auto height = rect->getShape().max_corner().y()
                    - rect->getShape().min_corner().y();

      out << YAML::BeginSeq << fmt(cell->getPosition().x() - width / 2.0)
          << fmt(cell->getPosition().y() - height / 2.0) << YAML::EndSeq;

      std::string  orient   = cell->getMirror() ? "F" : "";
      auto         rotation = cell->getRotation();
      const double eps      = 1e-6;
      if (std::abs(rotation - 0.0) < eps)
        orient += "N";
      else if (std::abs(rotation - 90.0) < eps)
        orient += "W";
      else if (std::abs(rotation - 180.0) < eps)
        orient += "S";
      else if (std::abs(rotation - 270.0) < eps)
        orient += "E";
      else {
        OPDB::logger->error("[LIDARYML] Unsupported rotation: {}", rotation);
        std::exit(1);
      }
      out << orient;
      out << YAML::EndSeq;
      out << YAML::EndMap;  // End settings map
      out << YAML::EndMap;  // End current instance map
    }
    out << YAML::EndMap;  // End instances map

    // schematic_placements
    out << YAML::Key << "schematic_placements" << YAML::Value << YAML::BeginMap;
    for (const auto& cellPair : design.getCells()) {
      const std::shared_ptr<Cell>& cell = cellPair.second;
      out << YAML::Key << cell->getName() << YAML::Value << YAML::BeginMap;

      auto rect  = std::dynamic_pointer_cast<Rect>(cell->getShape());
      auto width = rect->getShape().max_corner().x()
                   - rect->getShape().min_corner().x();
      auto height = rect->getShape().max_corner().y()
                    - rect->getShape().min_corner().y();

      out << YAML::Key << "x" << YAML::Value
          << fmt(cell->getPosition().x() - width / 2.0);
      out << YAML::Key << "y" << YAML::Value
          << fmt(cell->getPosition().y() - height / 2.0);
      out << YAML::Key << "xmin" << YAML::Value << YAML::Null;
      out << YAML::Key << "ymin" << YAML::Value << YAML::Null;
      out << YAML::Key << "xmax" << YAML::Value << YAML::Null;
      out << YAML::Key << "ymax" << YAML::Value << YAML::Null;
      out << YAML::Key << "dx" << YAML::Value << 0;
      out << YAML::Key << "dy" << YAML::Value << 0;
      out << YAML::Key << "port" << YAML::Value << "sw";
      out << YAML::Key << "rotation" << YAML::Value << fmt(cell->getRotation());
      out << YAML::Key << "mirror" << YAML::Value << cell->getMirror();
      out << YAML::EndMap;
    }
    out << YAML::EndMap;

    int connectionCount = 0;
    out << YAML::Key << "nets" << YAML::Value << YAML::BeginMap;
    for (const auto& connection : design.getNets()) {
      const std::shared_ptr<Net>& net  = connection.second;
      auto pins = net->getPins();
      if (pins.size() != 2) {
        OPDB::logger->error(
            "[LIDARYML] Invalid pins in net (expected 2, found {})",
            pins.size());
        exit(1);
      }

      std::string source
          = pins[0]->getHostCell()->getName() + "," + pins[0]->getName();
      std::string target
          = pins[1]->getHostCell()->getName() + "," + pins[1]->getName();

      out << YAML::Key << "n_" + std::to_string(connectionCount) << YAML::Value
          << YAML::BeginSeq << source << target << YAML::EndSeq;
      connectionCount++;
    }
    out << YAML::EndMap;


    std::unordered_set<std::string> visitedTypes;
    out << YAML::Key << "library" << YAML::Value << YAML::BeginMap;
    for (const auto& cellPair : design.getCells()) {
      const std::shared_ptr<Cell>& cell = cellPair.second;
      if (visitedTypes.count(cell->getType()))
        continue;
      visitedTypes.insert(cell->getType());

      out << YAML::Key << cell->getType() << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "property" << YAML::Value << YAML::Null;
      out << YAML::Key << "iloss" << YAML::Value << fmt(0.0);
      out << YAML::Key << "type" << YAML::Value << "CORE";
      out << YAML::Key << "origin" << YAML::Value << YAML::BeginSeq << fmt(0.0)
          << fmt(0.0) << YAML::EndSeq;

      auto rect  = std::dynamic_pointer_cast<Rect>(cell->getShape());
      auto width = rect->getShape().max_corner().x()
                   - rect->getShape().min_corner().x();
      auto height = rect->getShape().max_corner().y()
                    - rect->getShape().min_corner().y();

      out << YAML::Key << "size" << YAML::Value << YAML::BeginSeq << fmt(width)
          << fmt(height) << YAML::EndSeq;
      out << YAML::Key << "site" << YAML::Value << "core";
      
      out << YAML::Key << "pins" << YAML::Value << YAML::BeginMap;

      for (const auto& pinPair : cell->getPins()) {
        const std::shared_ptr<Pin>& pin = pinPair.second;
        out << YAML::Key << pin->getName() << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "pin_offset_x" << YAML::Value
            << fmt(pin->getPosition().x() + width / 2.0);
        out << YAML::Key << "pin_offset_y" << YAML::Value
            << fmt(pin->getPosition().y() + height / 2.0);
        out << YAML::Key << "pin_width" << YAML::Value << fmt(pin->getWidth());
        out << YAML::Key << "pin_orient" << YAML::Value
            << fmt(pin->getRotation());
        out << YAML::Key << "pin_layer" << YAML::Value << 1;
        out << YAML::EndMap;
      }
      out << YAML::EndMap;  // pins
      out << YAML::EndMap;  // cell type
    }
    out << YAML::EndMap;  // library

    out << YAML::EndMap;  // Root map

    std::ofstream outFile(liDARYmlOutPath);
    outFile << out.c_str();
    OPDB::logger->info("[LIDARYML] LiDARYml data written to file: {}", liDARYmlOutPath);
  }
};