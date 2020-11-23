/**
 * @file server/channel/src/ZoneGeometry.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Contains zone specific data types and classes that represent
 *  the geometry of a zone.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ZoneGeometry.h"

// Standard C++11 includes
#include <cmath>
#include <map>

// object includes
#include <QmpElement.h>

using namespace channel;

Point::Point() : x(0.f), y(0.f) {}

Point::Point(float xCoord, float yCoord) : x(xCoord), y(yCoord) {}

bool Point::operator==(const Point& other) const {
  return x == other.x && y == other.y;
}

bool Point::operator!=(const Point& other) const {
  return x != other.x || y != other.y;
}

float Point::GetDistance(const Point& other) const {
  float dSquared =
      (float)(std::pow((x - other.x), 2) + std::pow((y - other.y), 2));
  return std::sqrt(dSquared);
}

Line::Line() {}

Line::Line(const Point& a, const Point& b) : std::pair<Point, Point>(a, b) {}

Line::Line(float aX, float aY, float bX, float bY)
    : std::pair<Point, Point>(Point(aX, aY), Point(bX, bY)) {}

bool Line::operator==(const Line& other) const {
  return first == other.first && second == other.second;
}

bool Line::Intersect(const Line& other, Point& point, float& dist) const {
  const Point& src = other.first;
  const Point& dest = other.second;

  Point delta1(dest.x - src.x, dest.y - src.y);
  Point delta2(second.x - first.x, second.y - first.y);

  float det = -delta2.x * delta1.y + delta1.x * delta2.y;
  if (det == 0.f) {
    // If the determinate is zero, the lines are parallel and the point
    // of intersection is undefined
    return false;
  }

  float s =
      (-delta1.y * (src.x - first.x) + delta1.x * (src.y - first.y)) / det;
  float t = (delta2.x * (src.y - first.y) - delta2.y * (src.x - first.x)) / det;

  if (s < 0 || s > 1 || t < 0 || t > 1) return false;

  point.x = src.x + (t * delta1.x);
  point.y = src.y + (t * delta1.y);

  delta1.x = point.x - src.x;
  delta1.y = point.y - src.y;

  dist = (delta1.x * delta1.x) + (delta1.y * delta1.y);

  return true;
}

ZoneShape::ZoneShape() : IsLine(true), OneWay(false) {}

ZoneShape::~ZoneShape() {}

bool ZoneShape::Collides(const Line& path, Point& point, Line& surface) const {
  // If the path is outside of the boundary rectangle and doesn't cross
  // through the max/min boundary points, no collision can exist
  if (((path.first.x < Boundaries[0].x && path.second.x < Boundaries[0].x) ||
       (path.first.x > Boundaries[1].x && path.second.x > Boundaries[1].x)) &&
      ((path.first.y < Boundaries[0].y && path.second.y < Boundaries[0].y) ||
       (path.first.y > Boundaries[1].y && path.second.y > Boundaries[1].y))) {
    return false;
  }

  float dist = 0.f;
  std::map<float, std::pair<const Line*, Point>> collisions;
  for (const Line& s : Lines) {
    bool intersect = s.Intersect(path, point, dist);
    bool passThrough = false;

    if (intersect && OneWay) {
      // If the first point of the line being drawn is to the right of the
      // direction of the path, allow pass through
      if (((path.second.x - path.first.x) * (s.first.y - path.first.y) -
           (path.second.y - path.first.y) * (s.first.x - path.first.x)) < 0) {
        passThrough = true;
      }
    }

    if (intersect && !passThrough) {
      collisions[dist] = std::pair<const Line*, Point>(&s, point);
    }
  }

  // If a collision exists, retun true with the closest point and surface
  // in the output params
  if (collisions.size() > 0) {
    auto pair = collisions.begin()->second;
    point = pair.second;
    surface = *pair.first;
    return true;
  } else {
    return false;
  }
}

ZoneQmpShape::ZoneQmpShape() : ShapeID(0), InstanceID(0), Active(true) {}

ZoneQmpShape::~ZoneQmpShape() {}

bool ZoneQmpShape::Collides(const Line& path, Point& point,
                            Line& surface) const {
  if (Active) {
    return ZoneShape::Collides(path, point, surface);
  }

  return false;
}

ZoneSpotShape::ZoneSpotShape() {}

ZoneSpotShape::~ZoneSpotShape() {}

bool ZoneGeometry::Collides(const Line& path, Point& point, Line& surface,
                            std::shared_ptr<ZoneShape>& shape,
                            const std::set<uint32_t> disabledBarriers) const {
  std::map<float,
           std::pair<std::shared_ptr<ZoneShape>, std::pair<const Line*, Point>>>
      collisions;
  for (auto s : Shapes) {
    bool disabled = s->Element ? disabledBarriers.find(s->Element->GetID()) !=
                                     disabledBarriers.end()
                               : false;
    if (!disabled && s->Collides(path, point, surface)) {
      float dSquared = (float)(std::pow((path.first.x - point.x), 2) +
                               std::pow((path.first.y - point.y), 2));
      std::pair<const Line*, Point> p(&surface, point);
      collisions[dSquared] =
          std::pair<std::shared_ptr<ZoneShape>, std::pair<const Line*, Point>>(
              shape, p);
    }
  }

  // If a collision exists, return true with the closest point, surface
  // and shape in the output params
  if (collisions.size() > 0) {
    auto pair = collisions.begin()->second;
    point = pair.second.second;
    surface = *pair.second.first;
    shape = pair.first;
    return true;
  } else {
    return false;
  }
}

bool ZoneGeometry::Collides(const Line& path, Point& point) const {
  Line surface;
  std::shared_ptr<ZoneShape> shape;
  return Collides(path, point, surface, shape);
}
