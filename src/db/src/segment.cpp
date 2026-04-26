#include "segment.h"

#include "cell.h"
#include "log.h"
#include "net.h"
#include "opdb.h"
#include "pin.h"
#include "shape.h"

// Constructor with parameters
Segment::Segment(std::string               _name,
                 std::shared_ptr<Instance> instance,
                 Point                     _sourcePosition,
                 Point                     _targetPosition,
                 double                    _sourceRotation,
                 double                    _targetRotation,
                 bool                      _swap)
    : Instance(_name, instance),
      _sourcePosition(_sourcePosition),
      _targetPosition(_targetPosition),
      _sourceRotation(_sourceRotation),
      _targetRotation(_targetRotation),
      _position(Point(0, 0)),
      _rotation(0.0),
      _mirror(false),
      _swap(_swap)
{
  if (getInstType() == InstanceType::STRAIGHT_SEGMENT) {
    _position = Point((_sourcePosition.x() + _targetPosition.x()) / 2.0,
                      (_sourcePosition.y() + _targetPosition.y()) / 2.0);
    _rotation = fmod(atan2(_targetPosition.y() - _sourcePosition.y(),
                           _targetPosition.x() - _sourcePosition.x())
                         * 180.0 / M_PI,
                     360.0);
  } else {
    if (_swap) {
      std::swap(_sourcePosition, _targetPosition);
      std::swap(_sourceRotation, _targetRotation);
    }
    Point                center            = getShape()->getCenter();
    std::shared_ptr<Pin> sourcePin         = getPin("o1");
    std::shared_ptr<Pin> targetPin         = getPin("o2");
    Point                sourcePinPosition = sourcePin->getPosition();
    Point                targetPinPosition = targetPin->getPosition();
    double               sourcePinRotation = sourcePin->getRotation();
    double               targetPinRotation = targetPin->getRotation();

    // _mirror calculation (y-axis mirror)
    double crossProduct1 = sin(_targetRotation - _sourceRotation);
    double crossProduct2 = sin(targetPinRotation - sourcePinRotation);
    _mirror              = (crossProduct1 > 0) != (crossProduct2 > 0);
    // double sourceVectorX    = cos(_sourceRotation);
    // double sourceVectorY    = sin(_sourceRotation);
    // double targetVectorX    = cos(_targetRotation);
    // double targetVectorY    = sin(_targetRotation);
    // double sourcePinVectorX = cos(sourcePinRotation);
    // double sourcePinVectorY = sin(sourcePinRotation);
    // double targetPinVectorX = cos(targetPinRotation);
    // double targetPinVectorY = sin(targetPinRotation);
    // double crossProduct1
    //     = sourceVectorX * targetVectorY - sourceVectorY * targetVectorX;
    // double crossProduct2 = sourcePinVectorX * targetPinVectorY
    //                        - sourcePinVectorY * targetPinVectorX;
    // _mirror = (crossProduct1 > 0) != (crossProduct2 > 0);

    // _rotation calculation
    Point  newTargetPinPosition = targetPinPosition;
    double newTargetPinRotation = targetPinRotation;
    if (_mirror) {
      newTargetPinPosition = Point(targetPinPosition.x(),
                                   2 * center.y() - targetPinPosition.y());
      targetPinRotation    = -targetPinRotation;
    }
    _rotation = fmod(_targetRotation - targetPinRotation, 360.0);

    // _position calculation
    double angle_rad = _rotation * M_PI / 180.0;
    double rotatedX  = (newTargetPinPosition.x() - center.x()) * cos(angle_rad)
                      - (newTargetPinPosition.y() - center.y()) * sin(angle_rad)
                      + center.x();
    double rotatedY = (newTargetPinPosition.x() - center.x()) * sin(angle_rad)
                      + (newTargetPinPosition.y() - center.y()) * cos(angle_rad)
                      + center.y();
    newTargetPinPosition = Point(rotatedX, rotatedY);
    _position
        = Point(center.x() + _targetPosition.x() - newTargetPinPosition.x(),
                center.y() + _targetPosition.y() - newTargetPinPosition.y());
  }
}

// Static factory method to create a Segment
std::shared_ptr<Segment> Segment::createSegment(
    std::string               _name,
    std::shared_ptr<Instance> instance,
    Point                     _sourcePosition,
    Point                     _targetPosition,
    double                    _sourceRotation,
    double                    _targetRotation,
    bool                      _swap)
{
  auto seg = std::make_shared<Segment>(_name,
                                       instance,
                                       _sourcePosition,
                                       _targetPosition,
                                       _sourceRotation,
                                       _targetRotation,
                                       _swap);
  // Set host segment for each pin
  for (const auto& pin : seg->getPins()) {
    pin.second->setHostSeg(seg);  // Set host segment for the pin
  }
  return seg;
}

Point Segment::getSourcePosition() const
{
  return _sourcePosition;
}

double Segment::getSourceRotation() const
{
  return _sourceRotation;
}

Point Segment::getTargetPosition() const
{
  return _targetPosition;
}

double Segment::getTargetRotation() const
{
  return _targetRotation;
}

Point Segment::getPosition() const
{
  return _position;
}

double Segment::getRotation() const
{
  return _rotation;
}

bool Segment::getMirror() const
{
  return _mirror;
}

bool Segment::getSwap() const
{
  return _swap;
}
// ------------------------------
// Information Printing
// ------------------------------
std::string Segment::printInfo() const
{
  std::ostringstream oss;
  oss << "Segment: " << _name << "\n";
  oss << "  Type: " << _type << "\n";
  oss << "  Source: (" << _sourcePosition.x() << ", " << _sourcePosition.y()
      << ", " << _sourceRotation << ")\n";
  oss << "  Target: (" << _targetPosition.x() << ", " << _targetPosition.y()
      << ", " << _targetRotation << ")\n";
  oss << "  Center: (" << _position.x() << ", " << _position.y() << ", "
      << _rotation << ")\n";
  oss << "  Mirror: " << (_mirror ? "True" : "False") << "\n";
  oss << "  Swap: " << (_swap ? "True" : "False") << "\n";
  return oss.str();
}