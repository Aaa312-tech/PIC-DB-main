#include "defyml.h"

#include "cell.h"
#include "instance.h"
#include "log.h"
#include "opdb.h"
#include "segment.h"
#include "segmentGraph.h"
#include "shape.h"

DefYml::DefYml(const std::string& defInPath) : _raw(Yml(defInPath))
{
}

DefYml& DefYml::readDefYml(const std::string& defInPath)
{
  _raw = Yml(defInPath);
  return *this;
}

Yml DefYml::getDefYml() const
{
  return _raw;
}

std::shared_ptr<Design> DefYml::toDesign(std::shared_ptr<Library> library)
{
  std::shared_ptr<Design> design = std::make_shared<Design>();

  // Name
  if (_raw.exists("name")) {
    YAML::Node nameNode = _raw.getNode("name");
    if (nameNode.IsScalar()) {
      design->setName(nameNode.as<std::string>());
      OPDB::logger->info("[DEFYML] Design name: {}",
                         design->getName());  // Log the design name
    } else {
      OPDB::logger->error("[DEFYML] Invalid 'name' field, expected a string");
      exit(1);
    }
  } else {
    design->setName("default");
    OPDB::logger->warn("[DEFYML] 'name' field is missing, setting to default");
  }

  // Bounding Box
  if (_raw.exists("bbox")) {
    YAML::Node bbox = _raw.getNode("bbox");
    if (bbox.IsSequence()) {
      std::vector<Point> points;
      for (const auto& coordsList : bbox) {
        // Assuming each coordsList contains two values (x, y)
        if (coordsList.size() == 2) {
          double x = coordsList[0].as<double>();  // Get x value
          double y = coordsList[1].as<double>();  // Get y value
          Point  point(x, y);
          points.push_back(point);
        } else {
          OPDB::logger->error("[DEFYML] Invalid 'bbox' coordinate");
          exit(1);
        }
      }
      design->setBoundingBox(points);
      OPDB::logger->info("[DEFYML] Bounding box:");
      for (const auto& point : points) {
        OPDB::logger->info("[DEFYML]  [{}, {}]", point.x(), point.y());
      }
    } else {
      OPDB::logger->error(
          "[DEFYML] Invalid 'bbox' format, expected a sequence of two points");
      exit(1);
    }
  } else {
    OPDB::logger->warn("[DEFYML] 'bbox' field is missing");
  }

  // Instances
  if (_raw.exists("instances")) {
    YAML::Node instances = _raw.getNode("instances");
    if (!instances.IsMap()) {
      OPDB::logger->error(
          "[DEFYML] Invalid 'instances' format, expected a map");
      exit(1);
    }
    for (const auto& instance : instances) {
      std::string instanceName = instance.first.as<std::string>();
      YAML::Node  instanceNode = instance.second;
      if (instanceNode["component"].IsScalar()) {
        std::string instValue = instanceNode["component"].as<std::string>();
        std::shared_ptr<Instance> instance = library->getInstance(instValue);
        if (!instance) {
          OPDB::logger->error("[DEFYML] Instance '{}' not found in Library",
                              instValue);
          exit(1);
        }
        InstanceType instanceType = instance->getInstType();
        if (instanceType == InstanceType::CELL) {
          design->addCell(instanceName, instance);
          OPDB::logger->info("[DEFYML] Added cell: {} (Total cells: {})",
                             instanceName,
                             design->getCells().size());
        } else if (instanceType == InstanceType::STRAIGHT_SEGMENT) {
          // if (instanceNode["settings"].IsMap()) {
          //   if (instanceNode["settings"]["length"].IsScalar()) {
          //     double length =
          //     instanceNode["settings"]["length"].as<double>();
          //     // design->addSegment(instanceName,
          //     //                    Segment::createSegment(
          //     //                        instance, SegmentType::STRAIGHT,
          //     //                        length));
          //     // OPDB::logger->info(
          //     //     "Added straight segment: {} (Total segments: {})",
          //     //     instanceName,
          //     //     design->getSegments().size());
          //   } else if (instanceNode["settings"]["radius"].IsScalar()) {
          //     double radius =
          //     instanceNode["settings"]["radius"].as<double>();
          //     // design->addSegment(instanceName,
          //     //                    Segment::createSegment(
          //     //                        instance, SegmentType::BEND, 0.0,
          //     //                        radius));
          //     // OPDB::logger->info("Added bend segment: {} (Total segments:
          //     // {})",
          //     //                    instanceName,
          //     //                    design->getSegments().size());
          //   } else {
          //     OPDB::logger->error(
          //         "Missing 'length' or 'radius' in settings for segment "
          //         "'{}'",
          //         instanceName);
          //     exit(1);
          //   }
        } else if (instanceType == InstanceType::BEND_SEGMENT) {
        }
      } else {
        OPDB::logger->error("[DEFYML] Missing 'cell' field for instance '{}'",
                            instanceName);
        exit(1);
      }
    }
  } else {
    OPDB::logger->error("[DEFYML] 'instances' section missing");
    exit(1);
  }

  // Placements
  if (_raw.exists("placements")) {
    YAML::Node placements = _raw.getNode("placements");
    if (!placements.IsMap()) {
      OPDB::logger->error(
          "[DEFYML] Invalid 'placements' format, expected a map");
      exit(1);
    }
    const auto& cells = design->getCells();
    for (const auto& placement : placements) {
      std::string instanceName = placement.first.as<std::string>();
      YAML::Node  posNode      = placement.second;
      auto        cellIt       = cells.find(instanceName);
      if (cellIt == cells.end()) {
        OPDB::logger->warn(
            "[DEFYML] No cell or segment found for placement '{}'",
            instanceName);
        continue;
      }
      std::shared_ptr<Cell> component;
      std::string           componentType;
      if (cellIt != cells.end()) {
        component = cellIt->second;
      }
      if (posNode["x"].IsScalar() && posNode["y"].IsScalar()) {
        double x = posNode["x"].as<double>();
        double y = posNode["y"].as<double>();
        Point  position(x, y);
        double rotation = 0.0;
        if (posNode["rotation"].IsScalar()) {
          rotation = posNode["rotation"].as<double>();
        }
        bool mirror = false;
        if (posNode["mirror"].IsScalar()) {
          mirror = posNode["mirror"].as<bool>();
        }
        component->setPosition(position);
        component->setRotation(rotation);
        component->setMirror(mirror);
        OPDB::logger->info(
            "[DEFYML] Placed {} at position ({}, {}) with rotation {} and "
            "mirror {}",
            instanceName,
            x,
            y,
            rotation,
            mirror);
      } else {
        OPDB::logger->error(
            "[DEFYML] Invalid placement data for {} '{}'. Missing 'x' or 'y' "
            "values",
            componentType,
            instanceName);
        exit(1);
      }
    }
  } else {
    OPDB::logger->warn("[DEFYML] 'placements' section missing");
  }

  // Connections
  if (_raw.exists("connections")) {
    YAML::Node connections = _raw.getNode("connections");
    if (!connections.IsMap()) {
      OPDB::logger->error(
          "[DEFYML] Invalid 'connections' format, expected a map");
      exit(1);
    }
    for (const auto& connection : connections) {
      std::string source = connection.first.as<std::string>();
      std::string target = connection.second.as<std::string>();

      // Check the format of the connection (should be 'cell,pin')
      size_t sourceCommaPosition = source.find(',');
      size_t targetCommaPosition = target.find(',');
      if (sourceCommaPosition == std::string::npos
          || targetCommaPosition == std::string::npos) {
        OPDB::logger->error(
            "[DEFYML] Invalid connection format: '{}' -> '{}'. Expected "
            "'cell,pin' format",
            source,
            target);
        exit(1);
      }

      std::string name1    = source.substr(0, sourceCommaPosition);
      std::string pinName1 = source.substr(sourceCommaPosition + 1);
      std::string name2    = target.substr(0, targetCommaPosition);
      std::string pinName2 = target.substr(targetCommaPosition + 1);

      // bool isConnected = design->connect(name1, pinName1, name2, pinName2);
      bool isConnected = design->connectCells(name1, pinName1, name2, pinName2);
      if (!isConnected) {
        OPDB::logger->error("[DEFYML] Failed to connect {}:{} to {}:{}",
                            name1,
                            pinName1,
                            name2,
                            pinName2);
        continue;
      }

      OPDB::logger->info("[DEFYML] Connected {}:{} to {}:{}",
                         name1,
                         pinName1,
                         name2,
                         pinName2);
    }
  } else {
    OPDB::logger->warn("[DEFYML] 'connections' section missing");
  }
  return design;  // Return the design if no errors
}

void DefYml::fromDesignToYml(const Design&      design,
                             const std::string& defOutPath)
{
  YAML::Emitter out;

  out << YAML::BeginMap;

  // 1. Design Name
  out << YAML::Key << "name" << YAML::Value << design.getName();
  OPDB::logger->info("[DEFYML] Writing design name: {}", design.getName());

  // 2. Bounding Box
  std::vector<Point> points = design.getShape()->getPoints();
  out << YAML::Key << "bbox" << YAML::Value << YAML::BeginSeq;
  for (const auto& point : points) {
    out << YAML::BeginSeq << point.x() << point.y() << YAML::EndSeq;
  }
  out << YAML::EndSeq;
  OPDB::logger->info("[DEFYML] Writing bounding box points:");
  for (const auto& point : points) {
    OPDB::logger->info("  [{}, {}]", point.x(), point.y());
  }

  // 3. Instances
  int instSegmentCount = 0;
  out << YAML::Key << "instances" << YAML::Value << YAML::BeginMap;
  for (const auto& cellPair : design.getCells()) {
    const std::string&           instanceName = cellPair.first;
    const std::shared_ptr<Cell>& cell         = cellPair.second;
    out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "component" << YAML::Value << cell->getType();
    out << YAML::EndMap;  // End instance map
  }
  for (const auto& netPair : design.getNets()) {
    const std::string&          netName = netPair.first;
    const std::shared_ptr<Net>& net     = netPair.second;
    if (net->getSegmentGraph()) {
      const auto& edges = net->getSegmentGraph()->getEdges();
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              std::string instanceName = segment->getName();
              out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
              out << YAML::Key << "component" << YAML::Value
                  << segment->getType();
              out << YAML::EndMap;  // End instance map
              instSegmentCount++;
            }
          }
        }
      }
    }
  }
  out << YAML::EndMap;  // End instances map
  OPDB::logger->info("[DEFYML] Writing instances num: {} cells, {} segments",
                     design.getCells().size(),
                     instSegmentCount);

  // 4. Placements
  out << YAML::Key << "placements" << YAML::Value << YAML::BeginMap;
  for (const auto& cellPair : design.getCells()) {
    const std::string&           instanceName = cellPair.first;
    const std::shared_ptr<Cell>& cell         = cellPair.second;
    const Point&                 position     = cell->getPosition();
    double                       rotation     = cell->getRotation();
    bool                         mirror       = cell->getMirror();
    out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "x" << YAML::Value << position.x();
    out << YAML::Key << "y" << YAML::Value << position.y();
    out << YAML::Key << "rotation" << YAML::Value << rotation;
    out << YAML::Key << "mirror" << YAML::Value << mirror;
    out << YAML::EndMap;  // End placement map
  }
  out << YAML::EndMap;  // End placements map
  OPDB::logger->info("[DEFYML] Writing placements num: {} cells",
                     design.getCells().size());

  // 5. Connections
  out << YAML::Key << "connections" << YAML::Value << YAML::BeginMap;
  for (const auto& connection : design.getNets()) {
    const std::string&          netName = connection.first;
    const std::shared_ptr<Net>& net     = connection.second;
    auto                        pins    = net->getPins();

    // Start writing this net's connection
    out << YAML::Key << netName << YAML::Value << YAML::BeginMap;

    // Get source and target pins
    if (pins.size() < 2) {
      OPDB::logger->error(
          "[DEFYML] Invalid number of pins in connection (expected at least 2, "
          "found "
          "{})",
          pins.size());
      exit(1);
    }

    auto        pinIterator = pins.begin();
    std::string source;
    auto        pin1 = pinIterator->get();
    if (pin1->getHostCell()) {
      source = pin1->getHostCell()->getName() + "," + pin1->getName();
    } else {
      OPDB::logger->error("[DEFYML] Pin {} has no host cell or segment",
                          pin1->getName());
      exit(1);
    }

    pinIterator++;
    std::string target;
    auto        pin2 = pinIterator->get();
    if (pin2->getHostCell()) {
      target = pin2->getHostCell()->getName() + "," + pin2->getName();
    } else {
      OPDB::logger->error("[DEFYML] Pin {} has no host cell or segment",
                          pin2->getName());
      exit(1);
    }

    // Write source and target
    out << YAML::Key << "source" << YAML::Value << source;
    out << YAML::Key << "target" << YAML::Value << target;

    // Write segments if any
    if (net->getSegmentGraph()) {
      out << YAML::Key << "segments" << YAML::Value << YAML::BeginMap;

      const auto& edges = net->getSegmentGraph()->getEdges();
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          // Convert Edge to SegmentEdge to get Segment
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              std::string segmentName = segment->getName();

              // Start writing this segment
              out << YAML::Key << segmentName << YAML::Value << YAML::BeginMap;

              // Write source position
              Point  sourcePosition = segment->getSourcePosition();
              double sourceRot      = segment->getSourceRotation();
              out << YAML::Key << "source" << YAML::Value;
              out << YAML::BeginSeq << sourcePosition.x() << sourcePosition.y()
                  << sourceRot << YAML::EndSeq;

              // Write target position
              Point  targetPosition = segment->getTargetPosition();
              double targetRot      = segment->getTargetRotation();
              Point  position       = segment->getPosition();
              out << YAML::Key << "target" << YAML::Value;
              out << YAML::BeginSeq << targetPosition.x() << targetPosition.y()
                  << targetRot << YAML::EndSeq;

              // Write Swap
              out << YAML::Key << "swap" << YAML::Value << segment->getSwap();

              // TODO Write position, rotation,and mirror
              // out << YAML::Key << "x" << YAML::Value << position.x();
              // out << YAML::Key << "y" << YAML::Value << position.y();
              // out << YAML::Key << "rotation" << YAML::Value
              //    << segment->getRotation();
              // out << YAML::Key << "mirror" << YAML::Value
              //    << segment->getMirror();

              // End this segment
              out << YAML::EndMap;
            }
          }
        }
      }
      // End segments
      out << YAML::EndMap;
    }

    // End this net's connection
    out << YAML::EndMap;
  }
  out << YAML::EndMap;  // End connections map
  OPDB::logger->info("[DEFYML] Writing connections num: {}",
                     design.getNets().size());

  out << YAML::EndMap;

  std::ofstream outFile(defOutPath);
  outFile << out.c_str();

  OPDB::logger->info("[DEFYML] DefYml data written to file: {}", defOutPath);
}

void DefYml::fromDesignToGdsfactoryYml(const Design&      design,
                                       const std::string& defOutPath)
{
  YAML::Emitter out;

  out << YAML::BeginMap;

  // 1. Design Name
  out << YAML::Key << "name" << YAML::Value << design.getName();
  OPDB::logger->info("[DEFYML] Writing design name: {}", design.getName());

  // 2. Bounding Box
  std::vector<Point> points = design.getShape()->getPoints();
  out << YAML::Key << "bbox" << YAML::Value << YAML::BeginSeq;
  for (const auto& point : points) {
    out << YAML::BeginSeq << point.x() << point.y() << YAML::EndSeq;
  }
  out << YAML::EndSeq;
  OPDB::logger->info("[DEFYML] Writing bounding box points:");
  for (const auto& point : points) {
    OPDB::logger->info("[DEFYML]  [{}, {}]", point.x(), point.y());
  }

  // 3. Instances
  int instSegmentCount = 0;
  out << YAML::Key << "instances" << YAML::Value << YAML::BeginMap;
  for (const auto& cellPair : design.getCells()) {
    const std::string&           instanceName = cellPair.first;
    const std::shared_ptr<Cell>& cell         = cellPair.second;
    out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "component" << YAML::Value << cell->getType();
    out << YAML::EndMap;  // End instance map
  }

  for (const auto& netPair : design.getNets()) {
    const std::string&          netName = netPair.first;
    const std::shared_ptr<Net>& net     = netPair.second;
    if (net->getSegmentGraph()) {
      const auto& edges = net->getSegmentGraph()->getEdges();
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              std::string instanceName = segment->getName();
              out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
              out << YAML::Key << "component" << YAML::Value
                  << segment->getType();
              if (segment->getInstType() == InstanceType::STRAIGHT_SEGMENT) {
                double segmentLength
                    = std::sqrt((segment->getSourcePosition().x()
                                 - segment->getTargetPosition().x())
                                    * (segment->getSourcePosition().x()
                                       - segment->getTargetPosition().x())
                                + (segment->getSourcePosition().y()
                                   - segment->getTargetPosition().y())
                                      * (segment->getSourcePosition().y()
                                         - segment->getTargetPosition().y()));
                out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "length" << YAML::Value << segmentLength;
                out << YAML::EndMap;  // End settings map
                instSegmentCount++;
              }
              out << YAML::EndMap;  // End instance map
            }
          }
        }
      }
    }
  }
  out << YAML::EndMap;  // End instances map
  OPDB::logger->info("[DEFYML] Writing instances num: {} cells, {} segments",
                     design.getCells().size(),
                     instSegmentCount);

  // 4. Placements
  int placedSegmentCount = 0;
  out << YAML::Key << "placements" << YAML::Value << YAML::BeginMap;
  for (const auto& cellPair : design.getCells()) {
    const std::string&           instanceName = cellPair.first;
    const std::shared_ptr<Cell>& cell         = cellPair.second;
    const Point&                 position     = cell->getPosition();
    double                       rotation     = cell->getRotation();
    bool                         mirror       = cell->getMirror();
    out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "x" << YAML::Value << position.x();
    out << YAML::Key << "y" << YAML::Value << position.y();
    out << YAML::Key << "rotation" << YAML::Value << rotation;
    out << YAML::Key << "mirror" << YAML::Value << mirror;
    out << YAML::EndMap;  // End placement map
  }
  for (const auto& connection : design.getNets()) {
    const std::shared_ptr<Net>& net = connection.second;
    if (net->getSegmentGraph()) {
      auto edges = net->getSegmentGraph()->getEdges();
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              const std::string& instanceName = segment->getName();
              const Point&       position     = segment->getPosition();
              double             rotation     = segment->getRotation();
              bool               mirror       = segment->getMirror();
              out << YAML::Key << instanceName << YAML::Value << YAML::BeginMap;
              out << YAML::Key << "x" << YAML::Value << position.x();
              out << YAML::Key << "y" << YAML::Value << position.y();
              out << YAML::Key << "rotation" << YAML::Value << rotation;
              out << YAML::Key << "mirror" << YAML::Value << mirror;
              out << YAML::EndMap;  // End placement map
              placedSegmentCount++;
            }
          }
        }
      }
    }
  }
  out << YAML::EndMap;  // End placements map
  OPDB::logger->info("[DEFYML] Writing placements num: {} cells, {} segments",
                     design.getCells().size(),
                     placedSegmentCount);

  // 5. Connections
  int connectionCount = 0;
  out << YAML::Key << "connections" << YAML::Value << YAML::BeginMap;
  for (const auto& connection : design.getNets()) {
    const std::shared_ptr<Net>& net  = connection.second;
    auto                        pins = net->getPins();
    if (pins.size() != 2) {
      OPDB::logger->error(
          "[DEFYML] Invalid number of pins in connection (expected 2, "
          "found {})",
          pins.size());
      exit(1);
    }
    auto        pin1 = pins[0].get();
    auto        pin2 = pins[1].get();
    std::string source, target;

    if (net->getSegmentGraph()) {
      if (pin1->getHostCell()) {
        source = pin1->getHostCell()->getName() + "," + pin1->getName();
      } else {
        OPDB::logger->error("[DEFYML] Pin {} has no host cell",
                            pin1->getName());
        exit(1);
      }
      auto edges = net->getSegmentGraph()->getEdges();
      std::vector<std::shared_ptr<Segment>> segments;
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              segments.push_back(segment);
            }
          }
        }
      }
      for (auto it = segments.rbegin(); it != segments.rend(); ++it) {
        auto        segment     = *it;
        std::string segmentName = segment->getName();
        bool        swap        = segment->getSwap();
        if (swap) {
          target = segmentName + ",o1";
        } else {
          target = segmentName + ",o2";
        }
        out << YAML::Key << source << YAML::Value << target;
        connectionCount++;
        if (swap) {
          source = segmentName + ",o2";
        } else {
          source = segmentName + ",o1";
        }
      }
      if (pin2->getHostCell()) {
        target = pin2->getHostCell()->getName() + "," + pin2->getName();
      } else {
        OPDB::logger->error("[DEFYML] Pin {} has no host cell",
                            pin2->getName());
        exit(1);
      }
      out << YAML::Key << source << YAML::Value << target;
      connectionCount++;
    } else {
      if (pin1->getHostCell()) {
        source = pin1->getHostCell()->getName() + "," + pin1->getName();
      } else {
        OPDB::logger->error("[DEFYML] Pin {} has no host cell",
                            pin1->getName());
        exit(1);
      }
      if (pin2->getHostCell()) {
        target = pin2->getHostCell()->getName() + "," + pin2->getName();
      } else {
        OPDB::logger->error("[DEFYML] Pin {} has no host cell",
                            pin2->getName());
        exit(1);
      }
      out << YAML::Key << source << YAML::Value << target;
      connectionCount++;
    }
  }
  out << YAML::EndMap;  // End connections map
  OPDB::logger->info("[DEFYML] Writing connections num: {}", connectionCount);

  out << YAML::EndMap;

  std::ofstream outFile(defOutPath);
  outFile << out.c_str();

  OPDB::logger->info("[DEFYML] DefYml data written to file: {}", defOutPath);
}

void DefYml::printRaw() const
{
  _raw.print();
}