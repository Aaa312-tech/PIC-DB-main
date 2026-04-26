#pragma once
#include <iostream>

class CrossSection
{
 public:
  CrossSection() = default;
  CrossSection(std::string name, double width) : _name(name), _width(width) {}
  std::string getName() const { return _name; }
  void        setName(const std::string& name) { _name = name; }
  double      getWidth() const { return _width; }
  void        setWidth(double width) { _width = width; }

 private:
  std::string _name;
  double      _width;
};