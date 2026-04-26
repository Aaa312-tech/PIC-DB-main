#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

// #include <boost/geometry/index/rtree.hpp>
#include <algorithm>
#include <cstdint>
#include <iostream>

namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::d2::point_xy<double> Point;
typedef bg::model::box<Point>           _box;
// typedef std::pair<_box, int> _value;
// typedef bgi::rtree<_value, boost::geometry::index::quadratic<128>> _rtree;

enum class ShapeType
{
  NONE,
  RECT,
  CIRCULAR
};

#include <vector>

class Shape
{
 public:
  virtual ~Shape() = default;
  Shape()          = default;
  Shape(Point ref, double rot, ShapeType shapeType)
      : _position(ref), _rotation(rot), _shapeType(shapeType)
  {
  }

  ShapeType getShapeType() const { return _shapeType; }

  void addPoint(const Point& point) { _points.push_back(point); }

  bool removePoint(size_t index)
  {
    if (index < _points.size()) {
      _points.erase(_points.begin() + index);
      return true;
    }
    return false;  // Index out of range
  }

  const std::vector<Point> getPoints() const { return _points; }

  void setPoints(const std::vector<Point>& points) { _points = points; }

  Point getCenter() const
  {
    if (_points.empty()) {
      return Point(0, 0);  // Return origin if no points
    }

    double sumX = 0, sumY = 0;
    for (const auto& point : _points) {
      sumX += point.x();
      sumY += point.y();
    }

    double centerX = sumX / _points.size();
    double centerY = sumY / _points.size();

    return Point(centerX, centerY);
  }

 private:
  ShapeType          _shapeType;
  Point              _position;
  std::vector<Point> _points;
  double             _rotation;
};

class Rect : public Shape
{
 public:
  Rect() = default;

  // Constructor that accepts 2 to 4 points, calculates the bounding box
  Rect(Point ref, double rot, const std::vector<Point>& points)
      : Shape(ref,
              rot,
              ShapeType::RECT)  // Default pos as (0, 0), will be adjusted
  {
    setPoints(points);
    // Calculate the bounding box from the points
    calculateBoundingBox();
  }

  _box getShape() const { return _boxShape; }

 private:
  _box _boxShape;

  // Method to calculate the bounding box
  void calculateBoundingBox()
  {
    std::vector<Point> points = getPoints();
    if (points.size() < 2)
      return;

    double minX = points[0].get<0>(), maxX = points[0].get<0>();
    double minY = points[0].get<1>(), maxY = points[0].get<1>();

    for (const auto& point : points) {
      minX = std::min(minX, point.get<0>());
      maxX = std::max(maxX, point.get<0>());
      minY = std::min(minY, point.get<1>());
      maxY = std::max(maxY, point.get<1>());
    }

    _boxShape = _box(Point(minX, minY), Point(maxX, maxY));
  }
};

// class Circular : public Shape
// {
// public:
//     Circular() = default;
//     Circular(Point pos, double rot,double innerR, double outerR,
//              double startA, double endA)
//         : _innerRadius(innerR), _outerRadius(outerR),
//           _startAngle(startA), _endAngle(endA),
//           Shape(pos, rot, ShapeType::CIRCULAR) {}
//     double getInnerRadius() const
//     {
//         return _innerRadius;
//     }
//     double getOuterRadius() const
//     {
//         return _outerRadius;
//     }
//     double getStartAngle() const
//     {
//         return _startAngle;
//     }
//     double getEndAngle() const
//     {
//         return _endAngle;
//     }
// private:
//     ShapeType _type = ShapeType::CIRCULAR;
//     uint32_t _innerRadius;
//     uint32_t _outerRadius;
//     double _startAngle;
//     double _endAngle;
// };
