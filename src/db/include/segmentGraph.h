#pragma once
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "segment.h"

class Segment;

// Segment graph class, representing a Directed Acyclic Graph (DAG) of routing
// segments
class SegmentGraph
{
 public:
  // Forward declaration of nested Edge type
  class Edge;

  // Node class representing a vertex in the graph
  class Node
  {
   private:
    double _x;  // x coordinate
    double _y;  // y coordinate

    std::shared_ptr<Edge>
        _in_edge;  // Incoming edge (each node has at most one)
    std::vector<std::shared_ptr<Edge>> _out_edges;  // Outgoing edge list

   public:
    // Constructor
    Node(double x, double y) : _x(x), _y(y) {}

    // Coordinate accessors
    double getX() const { return _x; }
    double getY() const { return _y; }

    // Edge accessors
    std::shared_ptr<Edge> getInEdge() const { return _in_edge; }
    const std::vector<std::shared_ptr<Edge>>& getOutEdges() const
    {
      return _out_edges;
    }

    // Node type checks
    bool isRoot() const
    {
      return _in_edge == nullptr;
    }  // Root node (no incoming edge)
    bool isLeaf() const
    {
      return _out_edges.empty();
    }  // Leaf node (no outgoing edges)

    // Print node information
    std::string printInfo() const;

    // Friend class declarations
    friend class SegmentGraph;
    friend class Edge;
  };

  // Base class for edges representing directed edges in the graph
  class Edge
  {
   public:
    enum Type
    {
      SEGMENT,
      TRANSITION
    };

   protected:
    Type                  _type;    // Edge type
    std::shared_ptr<Node> _source;  // Source node
    std::shared_ptr<Node> _target;  // Target node

   public:
    // Constructor
    Edge(Type type, std::shared_ptr<Node> source, std::shared_ptr<Node> target)
        : _type(type), _source(source), _target(target)
    {
    }

    // Accessor for edge type
    Type getType() const { return _type; }

    // Accessors for source and target nodes
    std::shared_ptr<Node> getSource() const { return _source; }
    std::shared_ptr<Node> getTarget() const { return _target; }

    // Print edge information
    virtual std::string printInfo() const = 0;

    // Friend class declarations
    friend class SegmentGraph;
    friend class Node;
  };

  // Segment edge class, representing a segment connection in the graph
  class SegmentEdge : public Edge
  {
   private:
    std::shared_ptr<Segment> _segment;  // Associated segment object

   public:
    // Constructor
    SegmentEdge(std::shared_ptr<Node>    source,
                std::shared_ptr<Node>    target,
                std::shared_ptr<Segment> segment);

    // Accessor for the associated segment
    std::shared_ptr<Segment> getSegment() { return _segment; }

    // // Get the length of the edge
    // double getLength() const;

    // Print edge information
    std::string printInfo() const override;

    // Friend class declaration
    friend class SegmentGraph;
  };

  // Constructors and destructor
  SegmentGraph()  = default;
  ~SegmentGraph() = default;

  // Clear the graph structure
  void clear();

  // Node management
  std::shared_ptr<Node> createNode(double x, double y);
  void                  deleteNode(std::shared_ptr<Node> node);

  // Edge management
  std::shared_ptr<SegmentEdge> createSegmentEdge(
      std::shared_ptr<Node>    source,
      std::shared_ptr<Node>    target,
      std::shared_ptr<Segment> segment);
  void deleteEdge(std::shared_ptr<Edge> edge);

  // Traversal support
  const std::vector<std::shared_ptr<Node>>& getNodes() const { return _nodes; }
  const std::vector<std::shared_ptr<Edge>>& getEdges() const { return _edges; }

  // Retrieve special nodes
  std::vector<std::shared_ptr<Node>> getRootNodes() const;
  std::vector<std::shared_ptr<Node>> getLeafNodes() const;

  // Print graph information
  std::string printInfo() const;

 private:
  std::vector<std::shared_ptr<Node>> _nodes;  // All nodes
  std::vector<std::shared_ptr<Edge>> _edges;  // All edges
};
