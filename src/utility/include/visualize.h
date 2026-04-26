#pragma once
#include "database.h"
#include "log.h"

namespace OPDB {
void visualize(const std::string& lef_path,
               const std::string& def_path,
               const std::string& file_path,
               bool               output_gds = true)
{
  OPDB::logger->info(
      "[VISUALIZE] Visualize {} with {} and {} and output_gds: {}",
      file_path,
      lef_path,
      def_path,
      output_gds);
  std::string cmd
      = std::string("python ../scripts/visualization/visualize.py")
        + " --lef_path " + lef_path + " --def_path " + def_path
        + " --file_path " + file_path + " --output_gds "
        + (output_gds ? "True" : "False");
  int ret = std::system(cmd.c_str());
  if (ret != 0) {
    OPDB::logger->error("[VISUALIZE] Visualization failed with exit code: {}",
                        ret);
    exit(1);
  }
}

void visualizeDesign(const std::shared_ptr<Design>& design,
                     const std::string&             filePath,
                     bool                           drawPins     = true,
                     bool                           drawFlyWires = true)
{
  OPDB::logger->info(
      "[VISUALIZE] Visualize design {} to {}", design->getName(), filePath);

  // Calculate bounding box
  double minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;

  // Check cells
  for (const auto& cellPair : design->getCells()) {
    const auto& cell = cellPair.second;
    const auto& pos  = cell->getPosition();

    // Use getBoundingBox to get cell dimensions
    auto   bbox       = cell->getBoundingBox();
    double cellWidth  = bbox.first;
    double cellHeight = bbox.second;

    if (cellWidth > 0 && cellHeight > 0) {
      // Calculate cell boundaries considering rotation
      minX = std::min(minX, pos.x() - cellWidth / 2);
      minY = std::min(minY, pos.y() - cellHeight / 2);
      maxX = std::max(maxX, pos.x() + cellWidth / 2);
      maxY = std::max(maxY, pos.y() + cellHeight / 2);
    } else {
      // Fallback: use position only
      minX = std::min(minX, pos.x());
      minY = std::min(minY, pos.y());
      maxX = std::max(maxX, pos.x());
      maxY = std::max(maxY, pos.y());
    }
  }

  // Check segments in nets
  for (const auto& netPair : design->getNets()) {
    const auto& net = netPair.second;
    if (net->getSegmentGraph()) {
      const auto& edges = net->getSegmentGraph()->getEdges();
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              const auto& sourcePos = segment->getSourcePosition();
              const auto& targetPos = segment->getTargetPosition();

              // Use default segment width of 5 as requested
              double segmentWidth = 5.0;

              minX = std::min(
                  minX,
                  std::min(sourcePos.x(), targetPos.x()) - segmentWidth / 2);
              minY = std::min(
                  minY,
                  std::min(sourcePos.y(), targetPos.y()) - segmentWidth / 2);
              maxX = std::max(
                  maxX,
                  std::max(sourcePos.x(), targetPos.x()) + segmentWidth / 2);
              maxY = std::max(
                  maxY,
                  std::max(sourcePos.y(), targetPos.y()) + segmentWidth / 2);
            }
          }
        }
      }
    }
  }

  // Add some padding
  double padding = 100;
  minX -= padding;
  minY -= padding;
  maxX += padding;
  maxY += padding;

  double width  = maxX - minX;
  double height = maxY - minY;

  // Create SVG file
  std::ofstream svgFile(filePath);
  svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
          << "\" height=\"" << height << "\">\n";

  // Draw grid
  svgFile << "  <defs>\n";
  svgFile << "    <pattern id=\"grid\" width=\"50\" height=\"50\" "
             "patternUnits=\"userSpaceOnUse\">\n";
  svgFile << "      <path d=\"M 50 0 L 0 0 0 50\" fill=\"none\" "
             "stroke=\"#e0e0e0\" stroke-width=\"1\"/>\n";
  svgFile << "    </pattern>\n";
  svgFile << "  </defs>\n";
  svgFile << "  <rect width=\"100%\" height=\"100%\" fill=\"url(#grid)\"/>\n";

  // Draw segments with proper width
  for (const auto& netPair : design->getNets()) {
    const auto& net = netPair.second;
    if (net->getSegmentGraph()) {
      const auto& edges = net->getSegmentGraph()->getEdges();
      for (const auto& edge : edges) {
        if (edge->getType() == SegmentGraph::Edge::Type::SEGMENT) {
          auto segmentEdge
              = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(edge);
          if (segmentEdge) {
            auto segment = segmentEdge->getSegment();
            if (segment) {
              const auto& sourcePos = segment->getSourcePosition();
              const auto& targetPos = segment->getTargetPosition();

              // Use default segment width of 5 as requested
              double segmentWidth = 5.0;

              double x1 = sourcePos.x() - minX;
              double y1 = height - (sourcePos.y() - minY);
              double x2 = targetPos.x() - minX;
              double y2 = height - (targetPos.y() - minY);

              // Draw segment as a thick line
              svgFile << "  <line x1=\"" << x1 << "\" y1=\"" << y1 << "\" "
                      << "x2=\"" << x2 << "\" y2=\"" << y2 << "\" "
                      << "stroke=\"#0000ff\" stroke-width=\"" << segmentWidth
                      << "\"/>\n";
            }
          }
        }
      }
    }
  }

  // Draw fly wire
  if (drawFlyWires) {
    for (const auto& netPair : design->getNets()) {
      const auto& net  = netPair.second;
      auto        pins = net->getPins();
      if (pins.size() < 2) {
        OPDB::logger->error("[VISUALIZE] Net {} has less than 2 pins",
                            net->getName());
        exit(1);
      }
      if (pins.size() > 2) {
        OPDB::logger->error("[VISUALIZE] Net {} has more than 2 pins",
                            net->getName());
        exit(1);
      }
      for (size_t i = 0; i < pins.size() - 1; ++i) {
        const auto& pin1    = pins[i];
        const auto& pin2    = pins[i + 1];
        const auto& pin1Pos = pin1->getAbsolutePosition();
        const auto& pin2Pos = pin2->getAbsolutePosition();
        double      pin1X   = pin1Pos.x() - minX;
        double      pin1Y   = height - (pin1Pos.y() - minY);
        double      pin2X   = pin2Pos.x() - minX;
        double      pin2Y   = height - (pin2Pos.y() - minY);
        svgFile << "  <line x1=\"" << pin1X << "\" y1=\"" << pin1Y << "\" "
                << "x2=\"" << pin2X << "\" y2=\"" << pin2Y << "\" "
                << "stroke=\"#00ff00\" stroke-width=\"1\"/>\n";
      }
    }
  }

  // Draw cells with proper size
  for (const auto& cellPair : design->getCells()) {
    const auto& cellName = cellPair.first;
    const auto& cell     = cellPair.second;
    const auto& pos      = cell->getPosition();

    double centerX = pos.x() - minX;
    double centerY = height - (pos.y() - minY);

    // Use getBoundingBox to get cell dimensions
    auto   bbox       = cell->getBoundingBox();
    double cellWidth  = bbox.first;
    double cellHeight = bbox.second;

    if (cellWidth <= 0 || cellHeight <= 0) {
      OPDB::logger->error("[VISUALIZE] Invalid bounding box for cell {}",
                          cellName);
      exit(1);
    }

    // Draw cell as a rectangle
    svgFile << "  <rect x=\"" << centerX - cellWidth / 2 << "\" y=\""
            << centerY - cellHeight / 2 << "\" width=\"" << cellWidth
            << "\" height=\"" << cellHeight << "\" "
            << "fill=\"#ffcccc\" stroke=\"#ff0000\" stroke-width=\"1\"/>\n";

    // Draw cell name
    svgFile << "  <text x=\"" << centerX << "\" y=\"" << centerY + 5
            << "\" text-anchor=\"middle\" font-size=\"12\">" << cellName
            << "</text>\n";

    // Draw pins
    if (drawPins) {
      auto pins = cell->getPins();
      for (const auto& pinPair : pins) {
        // first letter should be o
        if (pinPair.first[0] != 'o') {
          continue;
        }
        const auto& pin    = pinPair.second;
        const auto& pinPos = pin->getAbsolutePosition();
        double      pinX   = pinPos.x() - minX;
        double      pinY   = height - (pinPos.y() - minY);

        // Get pin width
        double pinWidth = pin->getWidth();
        if (pinWidth <= 0) {
          OPDB::logger->error(
              "[VISUALIZE] Invalid pin width for pin {} of cell {}",
              pin->getName(),
              cellName);
          exit(1);
        }
        // Draw pin as a small circle
        svgFile
            << "  <circle cx=\"" << pinX << "\" cy=\"" << pinY << "\" r=\""
            << pinWidth / 2
            << "\" fill=\"#00ff00\" stroke=\"#008800\" stroke-width=\"1\"/>\n";
      }
    }
  }
  svgFile << "</svg>\n";
  svgFile.close();

  OPDB::logger->info("[VISUALIZE] Design visualization saved to {}", filePath);
}

}  // namespace OPDB