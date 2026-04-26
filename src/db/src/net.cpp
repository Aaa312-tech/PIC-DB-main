#include "net.h"

#include "cell.h"
#include "instance.h"
#include "log.h"
#include "pin.h"
#include "segment.h"
#include "segmentGraph.h"

// ------------------------------
// Constructor
// ------------------------------
Net::Net(std::string _name, Routing _routing) : _name(_name), _routing(_routing)
{
  _segmentGraph = std::make_shared<SegmentGraph>();
}

// ------------------------------
// Pin Management
// ------------------------------
std::string Net::getName() const
{
  return _name;
}

void Net::addPin(std::shared_ptr<Pin> pin)
{
  pin->setHostNet(shared_from_this());
  _pins.emplace_back(pin);
}

void Net::removePin(const std::string& cellName, const std::string& pinName)
{
  for (auto it = _pins.begin(); it != _pins.end(); ++it) {
    std::shared_ptr<Pin>  pin      = *it;
    std::shared_ptr<Cell> hostCell = pin->getHostCell();
    if (hostCell && hostCell->getName() == cellName
        && pin->getName() == pinName) {
      _pins.erase(it);
      break;
    }
  }
}

bool Net::hasPin(const std::string& cellName, const std::string& pinName)
{
  for (const auto& pin : _pins) {
    std::shared_ptr<Cell> hostCell = pin->getHostCell();
    if (hostCell && hostCell->getName() == cellName
        && pin->getName() == pinName) {
      return true;
    }
  }
  return false;
}

std::vector<std::shared_ptr<Pin>> Net::getPins() const
{
  return _pins;
}

int Net::getPinNum() const
{
  return _pins.size();
}

std::string Net::printPins() const
{
  std::ostringstream oss;
  for (const auto& pin : _pins) {
    oss << pin->printInfo();
  }
  return oss.str();
}

// ------------------------------
// Routing Management
// ------------------------------
Routing Net::getRouting() const
{
  return _routing;
}

void Net::setRouting(Routing routing)
{
  _routing = routing;
}

bool Net::isRouted() const
{
  // Validate segment graph existence
  if (!_segmentGraph) {
    OPDB::logger->error("Net::isRouted: segment graph does not exist.");
    return false;
  }

  const auto& nodes = _segmentGraph->getNodes();
  if (nodes.empty()) {
    OPDB::logger->error("Net::isRouted: segment graph has no nodes.");
    return false;
  }

  // Get root and leaf edges from the segment graph
  auto rootEdge = _segmentGraph->getEdges().front();
  auto leafEdge = _segmentGraph->getEdges().back();

  Point  rootSourcePosition;
  double rootSourceRotation;
  Point  leafTargetPosition;
  double leafTargetRotation;

  // Extract root edge information (must be a segment edge)
  if (rootEdge->getType() != SegmentGraph::Edge::Type::SEGMENT) {
    OPDB::logger->error("Net::isRouted: root edge is not a segment edge.");
    return false;
  }
  auto rootSegmentEdge
      = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(rootEdge);
  if (!rootSegmentEdge) {
    OPDB::logger->error("Net::isRouted: root edge is not a segment edge.");
    return false;
  }
  rootSourcePosition = rootSegmentEdge->getSegment()->getSourcePosition();
  rootSourceRotation = rootSegmentEdge->getSegment()->getSourceRotation();

  // Extract leaf edge information (must be a segment edge)
  if (leafEdge->getType() != SegmentGraph::Edge::Type::SEGMENT) {
    OPDB::logger->error("Net::isRouted: leaf edge is not a segment edge.");
    return false;
  }
  auto leafSegmentEdge
      = std::dynamic_pointer_cast<SegmentGraph::SegmentEdge>(leafEdge);
  if (!leafSegmentEdge) {
    OPDB::logger->error("Net::isRouted: leaf edge is not a segment edge.");
    return false;
  }
  leafTargetPosition = leafSegmentEdge->getSegment()->getTargetPosition();
  leafTargetRotation = leafSegmentEdge->getSegment()->getTargetRotation();

  // Get pin information
  auto   pin1         = _pins[0];
  auto   pin2         = _pins[1];
  Point  pin1Position = pin1->getAbsolutePosition();
  Point  pin2Position = pin2->getAbsolutePosition();
  double pin1Rotation = pin1->getAbsoluteRotation();
  double pin2Rotation = pin2->getAbsoluteRotation();

  // Define tolerances for position and rotation matching
  const double positionTolerance
      = 1e-6;  // Allow small floating point differences in position
  const double rotationTolerance
      = 1e-6;  // Allow small floating point differences in rotation

  // Helper function to normalize angle difference to [-180, 180] degrees
  auto normalizeAngleDifference = [](double angleDiff) {
    angleDiff = fmod(angleDiff, 360.0);
    if (angleDiff > 180.0)
      angleDiff -= 360.0;
    if (angleDiff < -180.0)
      angleDiff += 360.0;
    return angleDiff;
  };

  // Helper function to check if two rotations match (equal or 180 degrees
  // apart)
  auto rotationsMatch = [&](double rot1, double rot2) {
    double angleDiff = normalizeAngleDifference(rot1 - rot2);
    return (std::fabs(angleDiff) < rotationTolerance)
           || (std::fabs(std::fabs(angleDiff) - 180.0) < rotationTolerance);
  };

  // Check position matching in both possible orders
  bool positionsMatchOrder1
      = (std::fabs(rootSourcePosition.x() - pin1Position.x())
             < positionTolerance
         && std::fabs(rootSourcePosition.y() - pin1Position.y())
                < positionTolerance
         && std::fabs(leafTargetPosition.x() - pin2Position.x())
                < positionTolerance
         && std::fabs(leafTargetPosition.y() - pin2Position.y())
                < positionTolerance);

  bool positionsMatchOrder2
      = (std::fabs(rootSourcePosition.x() - pin2Position.x())
             < positionTolerance
         && std::fabs(rootSourcePosition.y() - pin2Position.y())
                < positionTolerance
         && std::fabs(leafTargetPosition.x() - pin1Position.x())
                < positionTolerance
         && std::fabs(leafTargetPosition.y() - pin1Position.y())
                < positionTolerance);

  // Check rotation matching for both position orders
  bool rotationMatchOrder1 = false;
  bool rotationMatchOrder2 = false;

  if (positionsMatchOrder1) {
    rotationMatchOrder1 = rotationsMatch(rootSourceRotation, pin1Rotation)
                          && rotationsMatch(leafTargetRotation, pin2Rotation);
  }

  if (positionsMatchOrder2) {
    rotationMatchOrder2 = rotationsMatch(rootSourceRotation, pin2Rotation)
                          && rotationsMatch(leafTargetRotation, pin1Rotation);
  }

  // The net is routed if either order has matching positions and rotations
  return rotationMatchOrder1 || rotationMatchOrder2;
}

void Net::addSegment(std::shared_ptr<Segment> segment)
{
  if (!segment) {
    OPDB::logger->error("Net::addSegment: segment is null.");
    return;
  }
  // TODO only for 2 pin net
  if (_pins.size() != 2) {
    OPDB::logger->error("Net::addSegment: only support 2 pin net, but got {}.",
                        _pins.size());
    return;
  }
  // Initialize segmentGraph if this is the first segment added to the net
  if (!_segmentGraph) {
    _segmentGraph = std::make_shared<SegmentGraph>();
  }
  // Extract geometric endpoints of the segment
  Point                               start = segment->getSourcePosition();
  Point                               end   = segment->getTargetPosition();
  std::shared_ptr<SegmentGraph::Node> startNode;
  std::shared_ptr<SegmentGraph::Node> endNode;
  const auto&                         nodes = _segmentGraph->getNodes();

  if (nodes.empty()) {
    // Create the root node using segment start
    startNode = _segmentGraph->createNode(start.x(), start.y());
  } else {
    // The net is assumed to be a 2-pin linear chain.
    auto leaves = _segmentGraph->getLeafNodes();

    if (leaves.size() != 1) {
      OPDB::logger->error(
          "Net::addSegment: expected exactly 1 leaf node, but got {}.",
          leaves.size());
      return;
    }

    if (!leaves.empty()) {
      // Use the existing leaf node as the starting point
      startNode = leaves.front();
    } else {
      // Fallback: create a new start node if graph is malformed
      OPDB::logger->error("No leaf node found. Creating a new start node.");
      startNode = _segmentGraph->createNode(start.x(), start.y());
      return;
    }
  }
  // Create a new node for the segment's end point
  endNode = _segmentGraph->createNode(end.x(), end.y());
  // Add a directed edge to the graph from startNode → endNode
  _segmentGraph->createSegmentEdge(startNode, endNode, segment);
}

std::shared_ptr<SegmentGraph> Net::getSegmentGraph() const
{
  return _segmentGraph;
}

// ------------------------------
// Info Display
// ------------------------------
std::string Net::printInfo() const
{
  std::ostringstream oss;
  oss << "+------------------------------+\n";
  oss << "| Net Info                     |\n";
  oss << "+------------------------------+\n";
  oss << "| Name:        " << _name << "\n";
  oss << "+------------------------------+\n";
  oss << "| Connected Pins:              |\n";
  oss << "+------------------------------+\n";
  for (const auto& pin : _pins) {
    oss << pin->printInfo();
  }

  oss << "+------------------------------+\n";
  oss << "| Connected Segments:          |\n";
  oss << "+------------------------------+\n";
  if (_segmentGraph) {
    oss << _segmentGraph->printInfo();
  } else {
    oss << "| No segments in this net.     |\n";
  }

  oss << "+------------------------------+\n";

  return oss.str();
}