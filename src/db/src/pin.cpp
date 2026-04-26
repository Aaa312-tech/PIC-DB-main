#include "pin.h"
// #include <corecrt_math_defines.h>
#include <cmath>

#include "cell.h"
#include "net.h"
#include "opdb.h"
#include "segment.h"

// ------------------------------
// Constructor
// ------------------------------
Pin::Pin(std::string  _name,
         double       _width,
         Point        _position,
         double       _rotation,
         CrossSection _xsection)
    : _name(_name),
      _width(_width),
      _position(_position),
      _rotation(_rotation),
      _xsection(_xsection)
{
}

// ------------------------------
// Host Associations
// ------------------------------
void Pin::setHostNet(std::shared_ptr<Net> net)
{
  _hostNet = net;
}

void Pin::setHostCell(std::shared_ptr<Cell> cell)
{
  _hostCell = cell;
}

void Pin::setHostSeg(std::shared_ptr<Segment> seg)
{
  _hostSeg = seg;
}

std::shared_ptr<Net> Pin::getHostNet() const
{
  return _hostNet;
}

std::shared_ptr<Cell> Pin::getHostCell() const
{
  return _hostCell;
}

std::shared_ptr<Segment> Pin::getHostSeg() const
{
  return _hostSeg;
}

// ------------------------------
// Basic Info
// ------------------------------
std::string Pin::getName() const
{
  return _name;
}

std::string Pin::printInfo() const
{
  std::ostringstream oss;
  oss << "Name: " << _name << "\n";

  if (_hostCell) {
    oss << "Host Cell: " << _hostCell->getName() << "\n";
  } else {
    oss << "Host Cell: [Not assigned]\n";
  }

  if (_hostSeg) {
    oss << "Host Seg: " << _hostSeg->getName() << "\n";
  } else {
    oss << "Host Seg: [Not assigned]\n";
  }

  if (_hostNet) {
    oss << "Host Net: " << _hostNet->getName() << "\n";
  } else {
    oss << "Host Net: [Not assigned]\n";
  }

  oss << "Position: (" << getAbsolutePosition().x() << ", "
      << getAbsolutePosition().y() << ")\n";
  oss << "Rotation: " << getAbsoluteRotation() << " degrees\n";

  return oss.str();
}
std::string Pin::getDoc() const
{
  return _doc;
}

void Pin::setDoc(const std::string& doc)
{
  _doc = doc;
}

double Pin::getWidth() const
{
  return _width;
}

void Pin::setWidth(double width)
{
  _width = width;
}

Point Pin::getPosition() const
{
  if (_hostCell) {
    Point center = _hostCell->getShape()->getCenter();
    return Point(_position.x() - center.x(), _position.y() - center.y());
  } else if (_hostSeg) {
    Point center = _hostSeg->getShape()->getCenter();
    return Point(_position.x() - center.x(), _position.y() - center.y());
  } else {
    return _position;
  }
}

Point Pin::getAbsolutePosition() const
{
  if (_hostCell) {
    Point  cellPos  = _hostCell->getPosition();
    double rotation = _hostCell->getRotation();
    bool   mirror   = _hostCell->getMirror();
    Point  center   = _hostCell->getShape()->getCenter();
    double radians  = rotation * M_PI / 180.0;
    if (!mirror) {
      double x_rotated = (_position.x() - center.x()) * cos(radians)
                         - (_position.y() - center.y()) * sin(radians);
      double y_rotated = (_position.x() - center.x()) * sin(radians)
                         + (_position.y() - center.y()) * cos(radians);
      return Point(cellPos.x() + x_rotated, cellPos.y() + y_rotated);
    } else {
      Point  newPosition = Point(_position.x(), 2 * center.y() - _position.y());
      double x_rotated   = (newPosition.x() - center.x()) * cos(radians)
                         - (newPosition.y() - center.y()) * sin(radians);
      double y_rotated = (newPosition.x() - center.x()) * sin(radians)
                         + (newPosition.y() - center.y()) * cos(radians);
      return Point(cellPos.x() + x_rotated, cellPos.y() + y_rotated);
    }
  } else if (_hostSeg) {
    Point  segPosition = _hostSeg->getPosition();
    double rotation    = _hostSeg->getRotation();
    bool   mirror      = _hostSeg->getMirror();
    Point  center      = _hostSeg->getShape()->getCenter();
    double radians     = rotation * M_PI / 180.0;
    if (!mirror) {
      double x_rotated = (_position.x() - center.x()) * cos(radians)
                         - (_position.y() - center.y()) * sin(radians);
      double y_rotated = (_position.x() - center.x()) * sin(radians)
                         + (_position.y() - center.y()) * cos(radians);
      return Point(segPosition.x() + x_rotated, segPosition.y() + y_rotated);
    } else {
      Point  newPosition = Point(_position.x(), 2 * center.y() - _position.y());
      double x_rotated   = (newPosition.x() - center.x()) * cos(radians)
                         - (newPosition.y() - center.y()) * sin(radians);
      double y_rotated = (newPosition.x() - center.x()) * sin(radians)
                         + (newPosition.y() - center.y()) * cos(radians);
      return Point(segPosition.x() + x_rotated, segPosition.y() + y_rotated);
    }
  } else {
    return _position;
  }
}

void Pin::setPosition(const Point& position)
{
  _position = position;
}

double Pin::getRotation() const
{
  return _rotation;
}

double Pin::getAbsoluteRotation() const
{
  if (_hostCell) {
    bool mirror = _hostCell->getMirror();
    if (!mirror) {
      return fmod(_hostCell->getRotation() + _rotation, 360.0);
    } else {
      return fmod(_hostCell->getRotation() - _rotation, 360.0);
    }
  } else if (_hostSeg) {
    bool mirror = _hostSeg->getMirror();
    if (!mirror) {
      return fmod(_hostSeg->getRotation() + _rotation, 360.0);
    } else {
      return fmod(_hostSeg->getRotation() - _rotation, 360.0);
    }
  } else {
    return _rotation;
  }
}

void Pin::setRotation(double rotation)
{
  _rotation = rotation;
}

// ------------------------------
// Cross Section
// ------------------------------
// Get cross-section
CrossSection Pin::getCrossSection() const
{
  return _xsection;
}

// Set cross-section
void Pin::setCrossSection(const CrossSection& xsection)
{
  _xsection = xsection;
}
