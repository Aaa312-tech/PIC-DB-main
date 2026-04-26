#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "instance.h"
#include "log.h"
#include "shape.h"
#include "xsection.h"

// ============================================================
// Class: Library
// Represents a collection of reusable design instances such as
// components and routing segments. Provides methods to manage
// and retrieve them by name.
// ============================================================
class Library
{
 public:
  // ------------------------------
  // Constructor
  // ------------------------------
  Library() = default;
  Library(std::string _name) : _name(_name) {}

  // ------------------------------
  // Basic Info
  // ------------------------------
  void        setName(const std::string& name) { _name = name; }
  std::string getName() const { return _name; }

  // ------------------------------
  // Instance Management
  // ------------------------------
  void addInstance(std::shared_ptr<Instance> inst)
  {
    _insts[inst->getName()] = inst;
  }

  void removeInstance(const std::string& instName) { _insts.erase(instName); }

  std::unordered_map<std::string, std::shared_ptr<Instance>> getInstances()
      const
  {
    return _insts;
  }

  int getInstanceNum() const { return _insts.size(); }

  std::shared_ptr<Instance> getInstance(const std::string& instName) const
  {
    auto it = _insts.find(instName);
    if (it != _insts.end()) {
      return it->second;
    }
    OPDB::logger->error("Instance {} not found in library {}", instName, _name);
    exit(1);
  }

  // ------------------------------
  // CrossSection Management
  // ------------------------------
  void addCrossSection(std::shared_ptr<CrossSection> xsec)
  {
    _xsecs[xsec->getName()] = xsec;
  }
  void removeCrossSection(const std::string& xsecName)
  {
    _xsecs.erase(xsecName);
  }
  std::unordered_map<std::string, std::shared_ptr<CrossSection>>
  getCrossSections() const
  {
    return _xsecs;
  }
  int getCrossSectionNum() const { return _xsecs.size(); }
  std::shared_ptr<CrossSection> getCrossSection(
      const std::string& xsecName) const
  {
    auto it = _xsecs.find(xsecName);
    return it != _xsecs.end() ? it->second
                              : nullptr;  // Return nullptr if not found
  }

 private:
  // ------------------------------
  // Member Variables
  // ------------------------------
  std::string _name;  // Library name
  std::unordered_map<std::string, std::shared_ptr<Instance>>
      _insts;  // Instances in Library
  std::unordered_map<std::string, std::shared_ptr<CrossSection>>
      _xsecs;  // Cross sections in Library
};
