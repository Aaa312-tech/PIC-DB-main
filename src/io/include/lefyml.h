#pragma once

#include <map>
#include <string>
#include <vector>

#include "library.h"
#include "yml.h"

class LefYml
{
 public:
  // Constructor
  LefYml() = default;

  LefYml(const std::string& lefInPath);

  // Read LefYml
  LefYml& readLefYml(const std::string& lefInPath);

  // Getter for LefYml object
  Yml getLefYml() const;

  // Convert YML data to Library object
  std::shared_ptr<Library> toLibrary();

  // Convert Library object to YML data
  static void fromLibraryToYml(const Library&     library,
                               const std::string& lefOutPath);

  // Print raw YML data
  void printRaw() const;

 private:
  Yml _raw;  // Raw YML data
};
