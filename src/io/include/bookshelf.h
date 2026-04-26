#pragma once

// C++ Standard Library
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

// Project Headers
#include "cell.h"
#include "design.h"
#include "log.h"
#include "net.h"
#include "pin.h"
#include "shape.h"

static std::string getBaseNameFromPath(const std::string& path)
{
  size_t pos1 = path.find_last_of('/');
  size_t pos2 = path.find_last_of('\\');
  size_t pos  = std::string::npos;

  if (pos1 == std::string::npos)
    pos = pos2;
  else if (pos2 == std::string::npos)
    pos = pos1;
  else
    pos = std::max(pos1, pos2);

  if (pos == std::string::npos)
    return path;
  return path.substr(pos + 1);
}

class Bookshelf
{
 public:
  // Constructor
  Bookshelf() = default;
  static void fromDesignToBookshelf(const Design&      design,
                                    int                rowNum,
                                    int                siteNum,
                                    int                coordinateOrigin,
                                    int                SubrowOrigin,
                                    int                siteHeight,
                                    int                siteWidth,
                                    int                siteSpacing,
                                    int                halo,
                                    int                halo_x,
                                    int                halo_y,
                                    const std::string& bookshelfOutPath)
  {
    // .aux file
    OPDB::logger->info("[Bookshelf] Generating Bookshelf .aux file: {}.aux ",
                       bookshelfOutPath);
    std::ofstream auxFile(bookshelfOutPath + ".aux");
    std::string   baseName = getBaseNameFromPath(bookshelfOutPath);
    auxFile << "RowBasedPlacement : " << baseName << ".nodes " << baseName
            << ".nets " << baseName << ".wts " << baseName << ".pl " << baseName
            << ".scl" << std::endl;
    auxFile.close();
    OPDB::logger->info("[Bookshelf] {}.aux file generated.", bookshelfOutPath);

    // .wts file
    OPDB::logger->info("[Bookshelf] Generating Bookshelf .wts file: {}.wts ",
                       bookshelfOutPath);
    std::ofstream wtsFile(bookshelfOutPath + ".wts");
    wtsFile << "UCLA wts 1.0" << std::endl;
    wtsFile.close();
    OPDB::logger->info("[Bookshelf] {}.wts file generated.", bookshelfOutPath);

    int numTerminals = 0;
    for (const auto& cellPair : design.getCells()) {
      if (cellPair.second->getName().find("gc") == 0) {
        numTerminals++;
      }
    }

    // .scl file
    // TODO check bounding box
    OPDB::logger->info("[Bookshelf] Generating Bookshelf .scl file: {}.scl ",
                       bookshelfOutPath);
    std::ofstream sclFile(bookshelfOutPath + ".scl");
    sclFile << "UCLA scl 1.0" << std::endl;
    sclFile << std::endl;
    sclFile << "NumRows : " << rowNum << std::endl;
    sclFile << std::endl;
    for (int i = 0; i < rowNum; i++) {
      sclFile << "CoreRow Horizontal" << std::endl;
      sclFile << "  Coordinate    :   " << coordinateOrigin + i * siteHeight
              << std::endl;
      sclFile << "  Height        :   " << siteHeight << std::endl;
      sclFile << "  Sitewidth     :    " << siteWidth << std::endl;
      sclFile << "  Sitespacing   :    " << siteSpacing << std::endl;
      sclFile << "  Siteorient    :    " << 1 << std::endl;
      sclFile << "  Sitesymmetry  :    " << 1 << std::endl;
      sclFile << "  SubrowOrigin  :    " << SubrowOrigin
              << "	NumSites  :  " << siteNum << std::endl;
      sclFile << "End" << std::endl;
    }
    sclFile.close();
    OPDB::logger->info("[Bookshelf] {}.scl file generated.", bookshelfOutPath);

    // .pl file
    // TODO do not support fixed cells
    OPDB::logger->info("[Bookshelf] Generating Bookshelf .pl file: {}.pl",
                       bookshelfOutPath);
    std::ofstream plFile(bookshelfOutPath + ".pl");
    plFile << "UCLA pl 1.0" << std::endl;
    plFile << std::endl;
    for (const auto& cellPair : design.getCells()) {
      std::shared_ptr<Cell>     cell     = cellPair.second;
      Point                     cellPos  = cell->getPosition();
      std::pair<double, double> cellBBox = cell->getBoundingBox();
      // left bottom corner position
      int cellX = int(cellPos.x() - cellBBox.first / 2);
      int cellY = int(cellPos.y() - cellBBox.second / 2);
      if (cellX < 0) {
        cellX = 0;
      }
      if (cellY < 0) {
        cellY = 0;
      }
      // TODO rotation caculation
      plFile << cell->getName() << "\t" << cellX << "\t" << cellY << "\t: N ";
      if (cell->getName().find("gc") == 0) {
        plFile << " FIXED";
      }
      plFile << std::endl;
    }
    plFile.close();
    OPDB::logger->info("[Bookshelf] {}.pl file generated.", bookshelfOutPath);

    // .nodes file
    // TODO do not support terminials
    OPDB::logger->info(
        "[Bookshelf] Generating Bookshelf .nodes file: {}.nodes ",
        bookshelfOutPath);
    std::ofstream nodesFile(bookshelfOutPath + ".nodes");
    nodesFile << "UCLA nodes 1.0" << std::endl;
    nodesFile << std::endl;
    nodesFile << "NumNodes :  \t" << design.getCells().size() << std::endl;
    nodesFile << "NumTerminals :  \t" << numTerminals << std::endl;
    for (const auto& cellPair : design.getCells()) {
      std::shared_ptr<Cell>  cell      = cellPair.second;
      std::shared_ptr<Shape> cellShape = cell->getShape();
      // TODO rotation will impact position
      std::shared_ptr<Rect> cellRect
          = std::dynamic_pointer_cast<Rect>(cellShape);
      double cellWidth = cellRect->getShape().max_corner().x()
                         - cellRect->getShape().min_corner().x();
      if (cellWidth < halo_x) {
        cellWidth = halo_x;
      }
      cellWidth += 2 * halo;
      double cellHeight = cellRect->getShape().max_corner().y()
                          - cellRect->getShape().min_corner().y();
      if (cellHeight < halo_y) {
        cellHeight = halo_y;
      }
      cellHeight += 2 * halo;
      nodesFile << "\t" << cell->getName() << "\t" << int(cellWidth) << "\t"
                << int(cellHeight) << "\t";
      if (cell->getName().find("gc") == 0) {
        nodesFile << "terminal";
      }
      nodesFile << std::endl;
    }
    nodesFile.close();
    OPDB::logger->info("[Bookshelf] {}.nodes file generated.",
                       bookshelfOutPath);

    // .nets file
    OPDB::logger->info("[Bookshelf] Generating Bookshelf .nets file: {}.nets ",
                       bookshelfOutPath);
    std::ofstream netsFile(bookshelfOutPath + ".nets");
    netsFile << "UCLA nets 1.0" << std::endl;
    netsFile << std::endl;
    netsFile << "NumNets : " << design.getNets().size() << std::endl;
    // 2 pin net
    netsFile << "NumPins : " << design.getNets().size() * 2 << std::endl;
    for (const auto& netPair : design.getNets()) {
      std::shared_ptr<Net> net = netPair.second;
      netsFile << "NetDegree : " << net->getPins().size() << "   "
               << net->getName() << std::endl;
      for (const auto& pin : net->getPins()) {
        std::shared_ptr<Cell> cell    = pin->getHostCell();
        std::string           pinType = "I";
        if (pin == net->getPins().front()) {
          pinType = "O";
        }
        Point pinPos = pin->getPosition();
        netsFile << "\t" << cell->getName() << "\t" << pinType << " :\t"
                 << pinPos.x() << "\t" << pinPos.y() << std::endl;
      }
    }
    netsFile.close();
    OPDB::logger->info("[Bookshelf] {}.nets file generated.", bookshelfOutPath);
  }

  static void fromBookshelfPlaceToDesign(Design&            design,
                                         const std::string& plFilePath)
  {
    std::ifstream plFile(plFilePath);
    if (!plFile.is_open()) {
      OPDB::logger->error("[Bookshelf] Failed to open pl file: {}", plFilePath);
      return;
    }

    std::string line;
    int         updatedCells  = 0;
    int         notFoundCells = 0;

    while (std::getline(plFile, line)) {
      if (line.empty() || line[0] == '#') {
        continue;
      }
      std::istringstream iss(line);
      std::string        cellName, colon, type;
      double             x, y;
      if (iss >> cellName >> x >> y >> colon >> type && colon == ":") {
        // TODO handle other rotation
        if (type == "N") {
          std::shared_ptr<Cell> cell = design.getCell(cellName);
          if (cell) {
            auto bbox = cell->getBoundingBox();
            // set to center position
            cell->setPosition(Point(x + bbox.first / 2, y + bbox.second / 2));
            cell->setPlacement(Placement::PLACED);
            updatedCells++;
          } else {
            OPDB::logger->warn("[Bookshelf] Cell not found: {}", cellName);
            notFoundCells++;
          }
        }
      }
    }
    plFile.close();
    OPDB::logger->info(
        "[Bookshelf] .pl file completed: {} cells updated, {} cells not found",
        updatedCells,
        notFoundCells);
  }
};
