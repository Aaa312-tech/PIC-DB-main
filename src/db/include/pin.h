#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "cell.h"
#include "segment.h"
#include "shape.h"
#include "xsection.h"

class Net;
class Cell;
class Segment;

// ============================================================
// Class: Pin
// Represents a pin (connection point) of a cell.
// ============================================================
class Pin
{
 public:
  // ------------------------------
  // Constructor
  // ------------------------------
  Pin(std::string  _name,
      double       _width,
      Point        _position = Point(0, 0),
      double       _rotation = 0.0,
      CrossSection _xsection = CrossSection());

  // ------------------------------
  // Host Associations
  // ------------------------------
  void setHostNet(std::shared_ptr<Net> net);      // Assign host net
  void setHostCell(std::shared_ptr<Cell> cell);   // Assign host cell
  void setHostSeg(std::shared_ptr<Segment> seg);  // Assign host segment

  std::shared_ptr<Net>     getHostNet() const;   // Get host net
  std::shared_ptr<Cell>    getHostCell() const;  // Assign host cell
  std::shared_ptr<Segment> getHostSeg() const;   // Assign host segment

  // ------------------------------
  // Basic Info
  // ------------------------------
  std::string getName() const;                 // Return pin name
  std::string printInfo() const;               // Print pin details
  std::string getDoc() const;                  // Return pin description
  void        setDoc(const std::string& doc);  // Set pin description
  double      getWidth() const;                // Get pin width
  void        setWidth(double width);          // Set pin width
  Point       getPosition() const;             // Get pin position
  Point       getAbsolutePosition() const;     // Get pin absolute position
  void        setPosition(const Point& pos);   // Set pin position
  double      getRotation() const;             // Get pin rotation angle
  double      getAbsoluteRotation() const;   // Get pin absolute rotation angle
  void        setRotation(double rotation);  // Set pin rotation angle
  // ------------------------------
  // Cross Section
  // ------------------------------
  CrossSection getCrossSection() const;                // Get cross-section
  void setCrossSection(const CrossSection& xsection);  // Set cross-section

 private:
  // ------------------------------
  // Member Variables
  // ------------------------------
  std::string              _name;      // Pin name
  std::string              _doc;       // Documentation or description
  double                   _width;     // Pin width
  std::shared_ptr<Net>     _hostNet;   // Pointer to connected net
  std::shared_ptr<Cell>    _hostCell;  // Pointer to host cell
  std::shared_ptr<Segment> _hostSeg;   // Pointer to host cell
  Point                    _position;  // Position of pin
  double                   _rotation;  // Rotation angle
  CrossSection             _xsection;  // Cross-section
};
