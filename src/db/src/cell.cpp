#include "cell.h"

#include "opdb.h"
#include "pin.h"

// Constructor that creates Cell from an existing Instance
Cell::Cell(std::string               _name,
           std::shared_ptr<Instance> instance,
           Point                     _position,
           double                    _rotation,
           bool                      _mirror,
           Placement                 _placement)
    : Instance(_name, instance),
      _position(_position),
      _rotation(_rotation),
      _mirror(_mirror),
      _placement(_placement)
{
}

std::shared_ptr<Cell> Cell::createCell(std::string               _name,
                                       std::shared_ptr<Instance> instance,
                                       Point                     _position,
                                       double                    _rotation,
                                       bool                      _mirror,
                                       Placement                 _placement)
{
  auto cell = std::make_shared<Cell>(
      _name, instance, _position, _rotation, _mirror, _placement);
  // Set host cell for each pin
  for (const auto& pin : cell->getPins()) {
    pin.second->setHostCell(cell);
  }
  return cell;
}

std::shared_ptr<Cell> Cell::cloneCell(std::shared_ptr<Cell> _cell)
{
  auto cell = std::make_shared<Cell>(_cell->getName(),
                                     std::dynamic_pointer_cast<Instance>(_cell),
                                     _cell->getPosition(),
                                     _cell->getRotation(),
                                     _cell->getMirror(),
                                     _cell->getPlacement());
  // Set host cell for each pin
  for (const auto& pin : cell->getPins()) {
    pin.second->setHostCell(cell);
  }
  return cell;
}

// Get bounding box as (width, height)
std::pair<double, double> Cell::getBoundingBox() const
{
  std::shared_ptr<Rect> rect = std::dynamic_pointer_cast<Rect>(_shape);
  if (rect) {
    double width
        = rect->getShape().max_corner().x() - rect->getShape().min_corner().x();
    double height
        = rect->getShape().max_corner().y() - rect->getShape().min_corner().y();
    // Convert rotation to radians
    double rot_rad = _rotation * M_PI / 180.0;
    // Calculate new width and height after rotation
    double new_width
        = std::abs(width * std::cos(rot_rad) + height * std::sin(rot_rad));
    double new_height
        = std::abs(width * std::sin(rot_rad) + height * std::cos(rot_rad));
    return std::make_pair(new_width, new_height);
  }
  return std::make_pair(0.0, 0.0);
}

// Set position of the cell
void Cell::setPosition(Point pos)
{
  _position = pos;
}

// Get the position of the cell
Point Cell::getPosition() const
{
  return _position;
}

// Set rotation angle of the cell
void Cell::setRotation(double rot)
{
  _rotation = rot;
}

// Get the rotation angle of the cell
double Cell::getRotation() const
{
  return _rotation;
}

// Set the mirror flag
void Cell::setMirror(bool mir)
{
  _mirror = mir;
}

// Get the mirror flag
bool Cell::getMirror() const
{
  return _mirror;
}

// Set placement status
void Cell::setPlacement(Placement placement)
{
  _placement = placement;
}

// Get placement status
Placement Cell::getPlacement() const
{
  return _placement;
}

std::string Cell::printInfo() const
{
  std::ostringstream oss;
  oss << "Info:\n";
  oss << "Name: " << _name << "\n";
  oss << "Type: " << _type << "\n";
  oss << "Position: (" << _position.x() << ", " << _position.y() << ")\n";
  oss << "Rotation: " << _rotation << "\n";
  oss << "Mirror: " << _mirror << "\n";
  oss << "Pins: " << printPins() << "\n";
  return oss.str();
}