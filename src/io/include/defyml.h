#pragma once

#include <map>
#include <string>
#include <vector>

#include "design.h"
#include "library.h"
#include "yml.h"

class DefYml
{
 public:
  // Constructor
  DefYml() = default;

  DefYml(const std::string& defInPath);

  // Read DefYml
  DefYml& readDefYml(const std::string& defInPath);

  // Getter for DefYml objectDefYml
  Yml getDefYml() const;

  // Convert YML data to Design object
  std::shared_ptr<Design> toDesign(std::shared_ptr<Library> library);

  // Convert Design object to YML data
  static void fromDesignToYml(const Design&      design,
                              const std::string& defOutPath);
  static void fromDesignToGdsfactoryYml(const Design&      design,
                                        const std::string& defOutPath);

  // Print raw YML data
  void printRaw() const;

 private:
  Yml _raw;  // Raw YML data
};
