#include "segmentGraph.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "instance.h"
#include "log.h"
// Implementation of Node class
std::string SegmentGraph::Node::printInfo() const
{
  std::ostringstream oss;
  oss << "Node at (" << _x << ", " << _y << ")" << std::endl;
  oss << "  In edge: " << (_in_edge ? "Yes" : "No") << std::endl;
  oss << "  Out edges: " << _out_edges.size() << std::endl;
  return oss.str();
}

// Implementation of SegmentEdge class
SegmentGraph::SegmentEdge::SegmentEdge(
    std::shared_ptr<SegmentGraph::Node> source,
    std::shared_ptr<SegmentGraph::Node> target,
    std::shared_ptr<Segment>            segment)
    : Edge(SegmentGraph::Edge::Type::SEGMENT, source, target), _segment(segment)
{
}

std::string SegmentGraph::SegmentEdge::printInfo() const
{
  std::ostringstream oss;
  oss << "SegmentEdge from node (" << _source->getX() << ", " << _source->getY()
      << ") to node (" << _target->getX() << ", " << _target->getY() << ")"
      << std::endl;
  // oss << "  Length: " << getLength() << std::endl;
  if (_segment) {
    oss << "  Associated segment info: " << std::endl;
    oss << _segment->printInfo();
  }
  return oss.str();
}

// double SegmentGraph::SegmentEdge::getLength() const
// {
//   if (!_source || !_target) {
//     return 0.0;
//   }
//   // TODO: Implement curved segment length
//   return 0.0;
// }

void SegmentGraph::clear()
{
  // Clear all edges and nodes
  _edges.clear();
  _nodes.clear();
}

std::shared_ptr<SegmentGraph::Node> SegmentGraph::createNode(double x, double y)
{
  auto node = std::make_shared<SegmentGraph::Node>(x, y);
  _nodes.push_back(node);
  return node;
}

void SegmentGraph::deleteNode(std::shared_ptr<SegmentGraph::Node> node)
{
  if (!node) {
    return;
  }

  // Delete all related edges
  if (node->_in_edge) {
    deleteEdge(node->_in_edge);
  }

  // Create a copy because deleting edges will modify _out_edges
  auto out_edges = node->_out_edges;
  for (auto edge : out_edges) {
    deleteEdge(edge);
  }

  // Remove the node from the node list
  auto it = std::find(_nodes.begin(), _nodes.end(), node);
  if (it != _nodes.end()) {
    _nodes.erase(it);
  }
}

std::shared_ptr<SegmentGraph::SegmentEdge> SegmentGraph::createSegmentEdge(
    std::shared_ptr<SegmentGraph::Node> source,
    std::shared_ptr<SegmentGraph::Node> target,
    std::shared_ptr<Segment>            segment)
{
  if (!source || !target) {
    OPDB::logger->error("Error: Source or target node is nullptr!");
    exit(1);
  }

  // Check whether the target node already has an incoming edge (to maintain DAG
  // property)
  if (target->_in_edge) {
    std::cerr << "Error: Target node already has an in-edge." << std::endl;
    exit(1);
  }

  auto segmentEdge
      = std::make_shared<SegmentGraph::SegmentEdge>(source, target, segment);
  _edges.push_back(segmentEdge);
  source->_out_edges.push_back(segmentEdge);
  target->_in_edge = segmentEdge;

  return segmentEdge;
}

void SegmentGraph::deleteEdge(std::shared_ptr<SegmentGraph::Edge> edge)
{
  if (!edge) {
    return;
  }

  // Remove from the source node's outgoing edges list
  auto& out_edges = edge->_source->_out_edges;
  auto  it        = std::find(out_edges.begin(), out_edges.end(), edge);
  if (it != out_edges.end()) {
    out_edges.erase(it);
  }

  // Clear the target node's incoming edge reference
  if (edge->_target->_in_edge == edge) {
    edge->_target->_in_edge.reset();
  }

  // Remove from the edge list
  auto it_edge = std::find(_edges.begin(), _edges.end(), edge);
  if (it_edge != _edges.end()) {
    _edges.erase(it_edge);
  }
}

std::vector<std::shared_ptr<SegmentGraph::Node>> SegmentGraph::getRootNodes()
    const
{
  std::vector<std::shared_ptr<Node>> roots;
  for (auto node : _nodes) {
    if (node->isRoot()) {
      roots.push_back(node);
    }
  }
  return roots;
}

std::vector<std::shared_ptr<SegmentGraph::Node>> SegmentGraph::getLeafNodes()
    const
{
  std::vector<std::shared_ptr<SegmentGraph::Node>> leaves;
  for (auto node : _nodes) {
    if (node->isLeaf()) {
      leaves.push_back(node);
    }
  }
  return leaves;
}

std::string SegmentGraph::printInfo() const
{
  std::ostringstream oss;
  oss << "Nodes: " << _nodes.size() << ", Edges: " << _edges.size() << "\n";

  // Print root nodes
  auto roots = getRootNodes();
  if (!roots.empty()) {
    oss << "Roots: ";
    for (size_t i = 0; i < roots.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << "(" << roots[i]->getX() << ", " << roots[i]->getY() << ")";
    }
    oss << "\n";
  }

  // Print leaf nodes
  auto leaves = getLeafNodes();
  if (!leaves.empty()) {
    oss << "Leaves: ";
    for (size_t i = 0; i < leaves.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << "(" << leaves[i]->getX() << ", " << leaves[i]->getY() << ")";
    }
    oss << "\n";
  }

  // Print edges in a concise format
  if (!_edges.empty()) {
    oss << "Segments:\n";
    for (size_t i = 0; i < _edges.size(); ++i) {
      const auto& edge    = _edges[i];
      auto        segEdge = std::dynamic_pointer_cast<SegmentEdge>(edge);
      if (segEdge) {
        oss << segEdge->getSegment()->printInfo();
      }
    }
  }
  return oss.str();
}