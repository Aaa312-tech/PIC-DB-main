#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "segmentGraph.h"
#include "shape.h"

class Pin;
class Segment;
class Instance;
class SegmentGraph;

// ============================================================
// Enum: Routing
// Defines the routing status of a net
// ============================================================
enum class Routing
{
  UNROUTED,  // Net is not routed yet
  ROUTED,    // Net is routed
  FIXED      // Net is fixed
};

// ============================================================
// Class: Net
// Represents an optical net (connection) that links pins
// across multiple cells.
// ============================================================
class Net : public std::enable_shared_from_this<Net>
{
 public:
  // ------------------------------
  // Constructor
  // ------------------------------
  Net(std::string _name,
      Routing _routing = Routing::UNROUTED);  // Create a net with a given name

  // ------------------------------
  // Pin Management
  // ------------------------------
  std::string getName() const;                   // Get the net name
  void        addPin(std::shared_ptr<Pin> pin);  // Add a pin to this net
  void        removePin(
             const std::string& cellName,
             const std::string& pinName);  // Remove a pin by cell & pin name
  bool hasPin(const std::string& cellName,
              const std::string& pinName);  // Check if a pin exists in this net

  std::vector<std::shared_ptr<Pin>> getPins()
      const;                      // Get all pins connected to this net
  int         getPinNum() const;  // Get the number of pins in this net
  std::string printPins() const;  // Print all pin details

  // ------------------------------
  // Routing Management
  // ------------------------------
  Routing getRouting() const;           // Get the routing status of this net
  void    setRouting(Routing routing);  // Set the routing status of this net
  bool    isRouted() const;             // Check if the net is routed
  void    addSegment(
         std::shared_ptr<Segment> segment);  // Add a segment to this net
  std::shared_ptr<SegmentGraph> getSegmentGraph()
      const;  // Get the segment graph

  //  ------------------------------
  //  Info Display
  //  ------------------------------
  std::string printInfo()
      const;  // Print net information (name + pins + segments)

 private:
  // ------------------------------
  // Member Variables
  // ------------------------------
  std::string _name;  // Net name
  std::vector<std::shared_ptr<Pin>>
      _pins;  // Pins (ports) connected to this net
  std::shared_ptr<SegmentGraph>
          _segmentGraph;  // Segments connected to this net
  Routing _routing;       // Routing status of this net
};