#pragma once

#include <cstdint>
#include <string>

#include "instance.h"
#include "shape.h"

// ============================================================
// Enum: Placement
// Defines the placement status of a cell
// ============================================================
enum class Placement
{
  UNPLACED,  // Not placed yet
  PLACED,    // Placed on the layout
  FIXED      // Fixed position, cannot be moved
};

// ============================================================
// Class: Cell
// Inherits from Instance and adds information specific to cells
// ============================================================
class Cell : public Instance
{
 public:
  // ------------------------------
  // Constructor & Copy
  // ------------------------------
  Cell() = default;

  Cell(std::string               _name,
       std::shared_ptr<Instance> instance,
       Point                     _position  = Point(0, 0),
       double                    _rotation  = 0.0,
       bool                      _mirror    = false,
       Placement                 _placement = Placement::UNPLACED);

  // ------------------------------
  // Factory Method for initialization
  // ------------------------------
  static std::shared_ptr<Cell> createCell(std::string               _name,
                                          std::shared_ptr<Instance> instance,
                                          Point     _position = Point(0, 0),
                                          double    _rotation = 0.0,
                                          bool      _mirror   = false,
                                          Placement _placement
                                          = Placement::UNPLACED);
  static std::shared_ptr<Cell> cloneCell(std::shared_ptr<Cell> _cell);

  // ------------------------------
  // Placement Info
  // ------------------------------
  std::pair<double, double> getBoundingBox() const;

  void  setPosition(Point pos);
  Point getPosition() const;

  void   setRotation(double rot);
  double getRotation() const;

  void setMirror(bool mir);
  bool getMirror() const;

  void      setPlacement(Placement placement);
  Placement getPlacement() const;

  std::string printInfo() const override;

 protected:
  // ------------------------------
  // Member Variables
  // ------------------------------
  Point     _position;   // Position on layout
  double    _rotation;   // Rotation angle
  bool      _mirror;     // Mirrored flag
  Placement _placement;  // Placement status (specific to Cell)
};
