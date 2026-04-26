#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cell.h"
#include "library.h"
#include "log.h"
#include "net.h"
#include "pin.h"
#include "segment.h"
#include "shape.h"

// ============================================================
// Class: Design
// Represents an entire design/layout, including cells,
// nets, pins/ports, and routing segments.
// ============================================================
class Design
{
 public:
  // ------------------------------
  // Constructor
  // ------------------------------
  Design() = default;
  Design(std::string _name, std::shared_ptr<Shape> _shape)
      : _name(_name), _shape(_shape)
  {
  }

  void        setName(const std::string& name) { _name = name; }
  std::string getName() const { return _name; }
  void        setShape(std::shared_ptr<Shape> shape) { _shape = shape; }

  void setBoundingBox(double width, double height)
  {
    Point              ref(0, 0);
    double             rot    = 0.0;
    std::vector<Point> points = {Point(0, 0), Point(width, height)};
    _shape                    = std::make_shared<Rect>(ref, rot, points);
  }

  void setBoundingBox(Point lowerLeft, Point upperRight)
  {
    Point              ref(0, 0);
    double             rot    = 0.0;
    std::vector<Point> points = {lowerLeft, upperRight};
    _shape                    = std::make_shared<Rect>(ref, rot, points);
  }

  void setBoundingBox(std::vector<Point> points)
  {
    Point  ref(0, 0);
    double rot = 0.0;
    _shape     = std::make_shared<Rect>(ref, rot, points);
  }

  std::shared_ptr<Shape> getShape() const { return _shape; }

  void addCell(std::string cellName, std::shared_ptr<Instance> instance)
  {
    std::shared_ptr<Cell> cell = Cell::createCell(cellName, instance);
    _cells[cellName]           = cell;
  }

  void addCell(std::shared_ptr<Cell> cell) { _cells[cell->getName()] = cell; }

  bool connectCells(std::string name1,
                    std::string pinName1,
                    std::string name2,
                    std::string pinName2)
  {
    std::shared_ptr<Pin> pin1 = nullptr;
    std::shared_ptr<Pin> pin2 = nullptr;
    if (_cells.find(name1) != _cells.end()) {
      pin1 = _cells[name1]->getPin(pinName1);
    }
    if (_cells.find(name2) != _cells.end()) {
      pin2 = _cells[name2]->getPin(pinName2);
    }
    if (!pin1) {
      OPDB::logger->error(
          "Error: Cell {} or pin {} not found!", name1, pinName1);
      return false;
    }
    if (!pin2) {
      OPDB::logger->error(
          "Error: Cell {} or pin {} not found!", name2, pinName2);
      return false;
    }
    static int  netIndex = 0;
    std::string netName  = "net_" + std::to_string(netIndex);
    netIndex++;
    auto newNet = std::make_shared<Net>(netName);
    newNet->addPin(pin1);
    newNet->addPin(pin2);
    addNet(newNet);
    return true;
  }

  // Get all Cells (returns unordered_map)
  std::unordered_map<std::string, std::shared_ptr<Cell>> getCells() const
  {
    return _cells;
  }

  // Get a single cell by name; returns nullptr if not found
  std::shared_ptr<Cell> getCell(const std::string& cellName) const
  {
    auto it = _cells.find(cellName);
    if (it != _cells.end()) {
      return it->second;
    }
    OPDB::logger->error("Error: Cell {} not found!", cellName);
    exit(1);
  }

  // addNet
  void addNet(std::shared_ptr<Net> net) { _nets[net->getName()] = net; }

  // Get a single net by name; returns nullptr if not found
  std::shared_ptr<Net> getNet(const std::string& netName) const
  {
    auto it = _nets.find(netName);
    if (it != _nets.end()) {
      return it->second;
    }
    OPDB::logger->error("Error: Net {} not found!", netName);
    exit(1);
  }

  std::unordered_map<std::string, std::shared_ptr<Net>> getNets() const
  {
    return _nets;
  }

  // void addPort(std::shared_ptr<Pin> port) { _ports.push_back(port); }
  // std::vector<std::shared_ptr<Pin>> getPorts() const { return _ports; }
  // void addNet(std::shared_ptr<Net> net) { _nets[net->getName()] = net; }
  // std::unordered_map<std::string, std::shared_ptr<Net>> getNets() const {
  // return _nets; }
  // void addSegment(std::shared_ptr<Segment> seg) { _segs.push_back(seg); }
  // void addSegment(std::string segName, std::shared_ptr<Segment> seg)
  // {
  //   seg->setName(segName);
  //   _segs[segName] = seg;
  // }

  // std::unordered_map<std::string, std::shared_ptr<Segment>> getSegments()
  // const
  // {
  //   return _segs;
  // }

 private:
  // ------------------------------
  // Member Variables
  // ------------------------------
  std::string            _name;   // Design name
  std::shared_ptr<Shape> _shape;  // Design boundary/shape
  std::unordered_map<std::string, std::shared_ptr<Cell>>
                                    _cells;  // Cell (indexed by name)
  std::vector<std::shared_ptr<Pin>> _ports;  // Top-level ports (I/O pins)
  std::unordered_map<std::string, std::shared_ptr<Net>> _nets;  // Nets by name
  // std::unordered_map<std::string, std::shared_ptr<Segment>>
  //     _segs;  // Segments by name
};