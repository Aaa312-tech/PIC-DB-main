#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "shape.h"

class Pin;

enum class InstanceType
{
  CELL,
  STRAIGHT_SEGMENT,
  BEND_SEGMENT
};

// ============================================================
// Class: Instance
// Represents an optical instance in the layout.
// Stores basic information, pins, and shape details.
// ============================================================
class Instance
{
 public:
  // ------------------------------
  // Constructor & Copy
  // ------------------------------
  Instance() = default;
  Instance(std::string            _name,
           std::string            _type,
           std::shared_ptr<Shape> _shape,
           InstanceType           _instType = InstanceType::CELL);
  Instance(std::string _name, std::shared_ptr<Instance> instance);

  // Create a copy (deep clone) of this instance
  // std::shared_ptr<Instance> create() const;
  // std::shared_ptr<Instance> create(const std::string& InstName) const;

  // ------------------------------
  // Basic Info
  // ------------------------------
  std::string getName() const;                   // Return instance name
  void        setName(const std::string& name);  // Set instance name
  std::string getType() const;                   // Return instance type
  void        setType(const std::string& type);  // Set instance type
  std::string getDoc() const;                    // Return instance description
  void        setDoc(const std::string& doc);    // Set instance description
  std::shared_ptr<Shape> getShape() const;       // Return instance shape
  void         setShape(std::shared_ptr<Shape> shape);  // Set instance shape
  InstanceType getInstType() const;                     // Return instance type

  // ------------------------------
  // Pin Management
  // ------------------------------
  void addPin(std::shared_ptr<Pin> pin);  // Add a pin to the instance
  std::unordered_map<std::string, std::shared_ptr<Pin>> getPins()
      const;                                            // Get all pins
  std::shared_ptr<Pin> getPin(std::string name) const;  // Find pin by name
  std::string          printPins() const;               // Print pin details
  virtual std::string  printInfo() const;               // Print instance info

 protected:
  // ------------------------------
  // Member Variables
  // ------------------------------
  std::string _name;  // Instance name
  std::string _type;  // Instance type
  std::string _doc;   // Documentation or description
  std::unordered_map<std::string, std::shared_ptr<Pin>> _pins;  // Map of pins
  std::shared_ptr<Shape> _shape;  // Geometrical shape
  InstanceType           _instType;
};