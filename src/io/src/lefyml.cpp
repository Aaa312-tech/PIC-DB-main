#include "lefyml.h"

#include <iostream>

#include "instance.h"
#include "log.h"
#include "pin.h"
#include "segment.h"
#include "shape.h"
#include "xsection.h"

LefYml::LefYml(const std::string& lefInPath) : _raw(Yml(lefInPath))
{
}

LefYml& LefYml::readLefYml(const std::string& lefInPath)
{
  _raw = Yml(lefInPath);
  return *this;
}

Yml LefYml::getLefYml() const
{
  return _raw;
}

std::shared_ptr<Library> LefYml::toLibrary()
{
  std::shared_ptr<Library> library = std::make_shared<Library>();

  // Header section
  if (_raw.exists("header")) {
    YAML::Node headerNode = _raw.getNode("header");
    if (headerNode.IsMap()) {
      if (headerNode["description"]) {  // Check if description exists
        library->setName(headerNode["description"].as<std::string>());
        OPDB::logger->info("[LEFYML] Library name: {}",
                           library->getName());  // Log the Library name
      } else {
        OPDB::logger->error(
            "[LEFYML] 'description' field is missing in 'header'. Setting name "
            "to default.");
        library->setName("default");
        exit(1);
      }
    } else {
      OPDB::logger->error("[LEFYML] Invalid 'header' field, expected a map.");
      exit(1);
    }
  } else {
    OPDB::logger->warn(
        "[LEFYML] Library name is missing in 'header', setting to default.");
    library->setName("default");
  }

  // xsections section
  if (_raw.exists("xsections")) {
    YAML::Node xsectionsNode = _raw.getNode("xsections");

    // Iterate through each cross-section type
    for (YAML::const_iterator it = xsectionsNode.begin();
         it != xsectionsNode.end();
         ++it) {
      std::string sectionName
          = it->first.as<std::string>();  // Cross-section name (e.g.,
                                          // "heater_metal")
      YAML::Node sectionNode
          = it->second;  // Properties of the cross-section (e.g., width, etc.)

      if (sectionNode.IsMap()) {
        // Process specific properties for each section
        if (sectionNode["width"]) {
          double width
              = sectionNode["width"].as<double>();  // Get the width property
          // Here you can create a cross-section object or add it to the Library
          std::shared_ptr<CrossSection> xsection
              = std::make_shared<CrossSection>(sectionName, width);
          library->addCrossSection(xsection);
          OPDB::logger->info("[LEFYML] Added cross-section: {} with width: {}",
                             sectionName,
                             width);
        } else {
          OPDB::logger->error(
              "[LEFYML] Missing 'width' property for cross-section: {}",
              sectionName);
          exit(1);
        }
      } else {
        OPDB::logger->error(
            "[LEFYML] Invalid format for cross-section '{}'. Expected a map.",
            sectionName);
        exit(1);
      }
    }
  } else {
    OPDB::logger->error(
        "[LEFYML] No 'xsections' found in the YAML. No cross-sections will be "
        "added.");
    exit(1);
  }

  // Blocks section
  if (_raw.exists("blocks")) {
    YAML::Node blocksNode = _raw.getNode("blocks");
    for (YAML::const_iterator blockIt = blocksNode.begin();
         blockIt != blocksNode.end();
         ++blockIt) {
      std::string blockName = blockIt->first.as<std::string>();
      YAML::Node  blockNode = blockIt->second;
      OPDB::logger->info("[LEFYML] Processing block: {}", blockName);

      InstanceType blockType       = InstanceType::CELL;  // default
      std::string  blockTypeString = "cell";
      if (blockNode["type"] && blockNode["type"].IsScalar()) {
        blockTypeString = blockNode["type"].as<std::string>();
        if (blockTypeString == "straight_segment") {
          blockType = InstanceType::STRAIGHT_SEGMENT;
        } else if (blockTypeString == "bend_segment") {
          blockType = InstanceType::BEND_SEGMENT;
        } else {
          blockType       = InstanceType::CELL;
          blockTypeString = "cell";
        }
      }
      OPDB::logger->info(
          "[LEFYML] Block '{}' type: {}", blockName, blockTypeString);

      // bbox
      // TODO Only support Rect shape now
      std::vector<Point> points;
      if (blockNode["bbox"]) {
        YAML::Node bboxNode = blockNode["bbox"];
        OPDB::logger->info("[LEFYML] Block '{}' bbox:", blockName);
        for (const auto& coordsList : bboxNode) {
          // Assuming each coordsList contains two values (x, y)
          if (coordsList.size() == 2) {
            double x = coordsList[0].as<double>();  // Get x value
            double y = coordsList[1].as<double>();  // Get y value
            Point  point(x, y);
            points.push_back(point);
            OPDB::logger->info("[LEFYML] [{},{}]",
                               std::to_string(point.x()),
                               std::to_string(point.y()));
          } else {
            OPDB::logger->error(
                "[LEFYML] Block '{}' has an invalid 'bbox' coordinate",
                blockName);
            exit(1);
          }
        }
      } else {
        OPDB::logger->error("[LEFYML] Block '{}' does not have a 'bbox' field.",
                            blockName);
        exit(1);
      }

      Point                  ref(0, 0);
      double                 rot   = 0.0;
      std::shared_ptr<Shape> shape = std::make_shared<Rect>(ref, rot, points);
      std::shared_ptr<Instance> newInst
          = std::make_shared<Instance>(blockName, blockName, shape, blockType);

      // doc
      if (blockNode["doc"]) {
        std::string doc = blockNode["doc"].as<std::string>();
        newInst->setDoc(doc);
        OPDB::logger->info("[LEFYML] Block '{}' doc: {}", blockName, doc);
      } else {
        OPDB::logger->error("[LEFYML] Block '{}' does not have a 'doc' field.",
                            blockName);
        exit(1);
      }

      // pins
      if (blockNode["pins"]) {
        YAML::Node pinsNode = blockNode["pins"];
        for (YAML::const_iterator pinIt = pinsNode.begin();
             pinIt != pinsNode.end();
             ++pinIt) {
          std::string pinName = pinIt->first.as<std::string>();
          YAML::Node  pinNode = pinIt->second;
          OPDB::logger->info("[LEFYML] Processing pin: {}", pinName);

          // doc
          std::string pinDoc = "";
          if (pinNode["doc"]) {
            pinDoc = pinNode["doc"].as<std::string>();
            OPDB::logger->info("[LEFYML] Pin '{}' doc: {}", pinName, pinDoc);
          } else {
            OPDB::logger->error(
                "[LEFYML] Pin '{}' does not have a 'doc' field.", pinName);
            exit(1);
          }

          // width
          double pinWidth = 0.5;
          if (pinNode["width"]) {
            pinWidth = pinNode["width"].as<double>();
            OPDB::logger->info("[LEFYML] Pin '{}' width: {}",
                               pinName,
                               std::to_string(pinWidth));
          } else {
            OPDB::logger->error(
                "[LEFYML] Pin '{}' does not have a 'width' field. Set to "
                "default 0.5",
                pinName);
            exit(1);
          }

          // xsection
          std::string xsectionName = "default";
          if (pinNode["xsection"]) {
            xsectionName = pinNode["xsection"].as<std::string>();
            OPDB::logger->info(
                "[LEFYML] Pin '{}' xsection: {}", pinName, xsectionName);
          } else {
            OPDB::logger->error(
                "[LEFYML] Pin '{}' does not have an 'xsection' field. Set to "
                "default",
                pinName);
            exit(1);
          }

          std::shared_ptr<CrossSection> xsection
              = library->getCrossSection(xsectionName);
          if (xsection == nullptr) {
            OPDB::logger->error(
                "[LEFYML] Pin '{}' has an undefined cross-section '{}'. Please "
                "define it in the 'xsections' section.",
                pinName,
                xsectionName);
            exit(1);
          }

          // xya
          Point  pinPos(0, 0);
          double pinRot = 0.0;
          if (pinNode["xya"]) {
            YAML::Node xyaNode = pinNode["xya"];
            if (xyaNode.IsSequence() && xyaNode.size() == 3) {
              double x = xyaNode[0].as<double>();
              double y = xyaNode[1].as<double>();
              pinRot   = xyaNode[2].as<double>();
              pinPos   = Point(x, y);
              OPDB::logger->info(
                  "[LEFYML] Pin '{}' position: ({}, {}), rotation: {}",
                  pinName,
                  std::to_string(x),
                  std::to_string(y),
                  std::to_string(pinRot));
            } else {
              OPDB::logger->error(
                  "[LEFYML] Pin '{}' has an invalid 'xya' field. Expected a "
                  "sequence of three values [x, y, angle].",
                  pinName);
              exit(1);
            }
          } else {
            OPDB::logger->error(
                "[LEFYML] Pin '{}' does not have an 'xya' field. Set to "
                "default (0,0,0).",
                pinName);
            exit(1);
          }
          newInst->addPin(std::make_shared<Pin>(
              pinName, pinWidth, pinPos, pinRot, *xsection));
          OPDB::logger->info(
              "[LEFYML] Added pin: {} to instance: {}", pinName, blockName);
        }
      } else {
        OPDB::logger->error("[LEFYML] Block '{}' does not have a 'pins' field.",
                            blockName);
        exit(1);
      }

      library->addInstance(newInst);
      OPDB::logger->info("[LEFYML] Added block as instance: {}", blockName);
    }
  } else {
    OPDB::logger->error("[LEFYML] No 'blocks' section found in the YAML.");
    exit(1);
  }
  return library;
}

void LefYml::fromLibraryToYml(const Library&     library,
                              const std::string& lefOutPath)
{
  YAML::Emitter out;

  out << YAML::BeginMap;

  // 1. Blocks Section
  out << YAML::Key << "blocks" << YAML::Value << YAML::BeginMap;
  for (const auto& instPair : library.getInstances()) {
    const std::shared_ptr<Instance>& inst      = instPair.second;
    const std::string&               blockName = inst->getName();
    out << YAML::Key << blockName << YAML::Value << YAML::BeginMap;
    // instance type
    InstanceType instType = inst->getInstType();
    if (instType == InstanceType::CELL) {
      out << YAML::Key << "type" << YAML::Value << "cell";
    } else if (instType == InstanceType::STRAIGHT_SEGMENT) {
      out << YAML::Key << "type" << YAML::Value << "straight_segment";
    } else if (instType == InstanceType::BEND_SEGMENT) {
      out << YAML::Key << "type" << YAML::Value << "bend_segment";
    }
    // bbox
    out << YAML::Key << "bbox" << YAML::Value << YAML::BeginSeq;
    for (const auto& point : inst->getShape()->getPoints()) {
      out << YAML::BeginSeq << YAML::Value << point.x() << YAML::Value
          << point.y() << YAML::EndSeq;
    }
    out << YAML::EndSeq;  // End bbox sequence
    // doc
    out << YAML::Key << "doc" << YAML::Value << inst->getDoc();
    // parameters
    out << YAML::Key << "parameters" << YAML::Value << YAML::BeginMap;
    out << YAML::EndMap;  // End parameters map
    // pins
    out << YAML::Key << "pins" << YAML::Value << YAML::BeginMap;
    for (const auto& pinPair : inst->getPins()) {
      const std::shared_ptr<Pin>& pin = pinPair.second;
      out << YAML::Key << pin->getName() << YAML::Value << YAML::BeginMap;
      // alias
      out << YAML::Key << "alias" << YAML::Value << YAML::Null;
      //  doc
      out << YAML::Key << "doc" << YAML::Value << pin->getDoc();
      // width
      out << YAML::Key << "width" << YAML::Value << pin->getWidth();
      // xsection
      out << YAML::Key << "xsection" << YAML::Value
          << pin->getCrossSection().getName();
      // xya
      out << YAML::Key << "xya" << YAML::Value << YAML::BeginSeq;
      out << pin->getPosition().x() << pin->getPosition().y()
          << pin->getRotation();
      out << YAML::EndSeq;  // End xya sequence
      out << YAML::EndMap;  // End pin map
    }
    out << YAML::EndMap;  // End pins map

    // settings
    out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Properties" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "doc" << YAML::Value << "";
    out << YAML::Key << "max" << YAML::Value << 0.0;
    out << YAML::Key << "min" << YAML::Value << 0.0;
    out << YAML::Key << "type" << YAML::Value << "float";
    out << YAML::Key << "unit" << YAML::Value << YAML::Null;
    out << YAML::Key << "value" << YAML::Value << 0.0;
    out << YAML::EndMap;  // End Properties map
    out << YAML::Key << "info" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "doc" << YAML::Value
        << "dictionary that includes derived properties, simulation_settings, "
           "settings";
    out << YAML::Key << "max" << YAML::Value << 0.0;
    out << YAML::Key << "min" << YAML::Value << 0.0;
    out << YAML::Key << "type" << YAML::Value << "float";
    out << YAML::Key << "unit" << YAML::Value << YAML::Null;
    out << YAML::Key << "value" << YAML::Value << 0.0;
    out << YAML::EndMap;  // End info map
    out << YAML::EndMap;  // End settings map
    out << YAML::EndMap;  // End block map
    OPDB::logger->info("[LEFYML] Writing block: {}", blockName);
  }
  out << YAML::EndMap;  // End blocks map

  // 2. Header Section
  out << YAML::Key << "header" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "description" << YAML::Value << library.getName();
  out << YAML::EndMap;  // End header map
  OPDB::logger->info("[LEFYML] Writing Library name: {}", library.getName());

  // 3. xsections Section
  out << YAML::Key << "xsections" << YAML::Value << YAML::BeginMap;
  for (const auto& xsectionPair : library.getCrossSections()) {
    const std::string&                   sectionName = xsectionPair.first;
    const std::shared_ptr<CrossSection>& xsection    = xsectionPair.second;

    out << YAML::Key << sectionName << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "width" << YAML::Value << xsection->getWidth();
    out << YAML::EndMap;  // End cross-section map
    OPDB::logger->info("[LEFYML] Writing cross-section: {} with width: {}",
                       sectionName,
                       xsection->getWidth());
  }
  out << YAML::EndMap;  // End xsections map

  out << YAML::EndMap;  // End the main map

  // Save the YAML output to file
  std::ofstream outFile(lefOutPath);
  outFile << out.c_str();
  outFile.close();

  OPDB::logger->info("[LEFYML] LefYml data written to file: {}", lefOutPath);
}

void LefYml::printRaw() const
{
  _raw.print();
}