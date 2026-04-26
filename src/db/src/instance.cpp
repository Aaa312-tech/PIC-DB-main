#include "instance.h"

#include "log.h"
#include "pin.h"

// ------------------------------
// Constructor & Copy
// ------------------------------

Instance::Instance(std::string            _name,
                   std::string            _type,
                   std::shared_ptr<Shape> _shape,
                   InstanceType           _instType)
    : _name(_name), _type(_type), _shape(_shape), _instType(_instType)
{
}
Instance::Instance(std::string _name, std::shared_ptr<Instance> instance)
    : _name(_name),
      _type(instance->getType()),
      _doc(instance->getDoc()),
      _shape(instance->getShape()),
      _instType(instance->getInstType())
{
  for (const auto& pin : instance->getPins()) {
    _pins[pin.first] = std::make_shared<Pin>(*pin.second);
  }
}

// ------------------------------
// Basic Info
// ------------------------------

std::string Instance::getName() const
{
  return _name;
}

void Instance::setName(const std::string& name)
{
  _name = name;
}

std::string Instance::getType() const
{
  return _type;
}

void Instance::setType(const std::string& type)
{
  _type = type;
}

std::string Instance::getDoc() const
{
  return _doc;
}

void Instance::setDoc(const std::string& doc)
{
  _doc = doc;
}

std::shared_ptr<Shape> Instance::getShape() const
{
  return _shape;
}

void Instance::setShape(std::shared_ptr<Shape> shape)
{
  _shape = shape;
}

InstanceType Instance::getInstType() const
{
  return _instType;
}

// ------------------------------
// Pin Management
// ------------------------------

void Instance::addPin(std::shared_ptr<Pin> pin)
{
  _pins[pin->getName()] = pin;
}

std::unordered_map<std::string, std::shared_ptr<Pin>> Instance::getPins() const
{
  return _pins;
}

std::shared_ptr<Pin> Instance::getPin(std::string name) const
{
  auto it = _pins.find(name);
  if (it != _pins.end()) {
    return it->second;  // Return the pin if found
  }
  OPDB::logger->error("Error: Pin {} not found!", name);
  exit(1);
}

std::string Instance::printPins() const
{
  std::ostringstream oss;
  if (_pins.empty()) {
    oss << "  + [No pins assigned]\n";
    return oss.str();
  }

  for (const auto& pin : _pins) {
    oss << "  + Pin: " << pin.first << "\n";
    std::string        pinInfo = pin.second->printInfo();
    std::istringstream pinInfoStream(pinInfo);
    std::string        line;
    while (std::getline(pinInfoStream, line)) {
      oss << "    " << line << "\n";
    }
  }
  return oss.str();
}

std::string Instance::printInfo() const
{
  std::ostringstream oss;
  oss << "Instance: " << _name << "\n";
  oss << "  Type: " << _type << "\n";
  oss << "  Pins:\n";
  oss << printPins();
  return oss.str();
}