#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "instance.h"
#include "net.h"
#include "pin.h"
#include "shape.h"

class Instance;

// ============================================================
// Class: Segment
// Inherits from Instance without adding new functionality
// ============================================================
class Segment : public Instance
{
 public:
  // ------------------------------
  // Constructor & Copy
  // ------------------------------
  Segment() = default;

  Segment(std::string               _name,
          std::shared_ptr<Instance> instance,
          Point                     _sourcePosition = Point(0, 0),
          Point                     _targetPosition = Point(0, 0),
          double                    _sourceRotation = 0.0,
          double                    _targetRotation = 0.0,
          bool                      _swap           = false);

  // Static factory method to create a Segment
  static std::shared_ptr<Segment> createSegment(
      std::string               _name,
      std::shared_ptr<Instance> instance,
      Point                     _sourcePosition = Point(0, 0),
      Point                     _targetPosition = Point(0, 0),
      double                    _sourceRotation = 0.0,
      double                    _targetRotation = 0.0,
      bool                      _swap           = false);

  Point  getSourcePosition() const;
  double getSourceRotation() const;
  Point  getTargetPosition() const;
  double getTargetRotation() const;
  Point  getPosition() const;
  double getRotation() const;
  bool   getMirror() const;
  bool   getSwap() const;

  // ------------------------------
  // Information Printing
  // ------------------------------
  std::string printInfo() const override;

 protected:
  // ------------------------------
  // Member Variables
  // ------------------------------
  Point  _sourcePosition;
  double _sourceRotation;
  Point  _targetPosition;
  double _targetRotation;
  Point  _position;
  double _rotation;
  bool   _mirror;
  bool   _swap;
};
