/**
 * @file server/channel/src/ZoneGeometry.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Contains zone specific data types and classes that represent
 *  the geometry of a zone.
 *
 * This file is part of the Channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef SERVER_CHANNEL_SRC_ZONEGEOMETRY_H
#define SERVER_CHANNEL_SRC_ZONEGEOMETRY_H

// libcomp Includes
#include <CString.h>

// Standard C++11 includes
#include <array>
#include <list>

namespace channel
{

/**
 * Simple X, Y coordinate point.
 */
class Point
{
public:
    /**
     * Create a new point at 0, 0
     */
    Point();

    /**
     * Create a new point at the specified coordinates
     * @param xCoord X coordinate to set
     * @param yCoord Y coordinate to set
     */
    Point(float xCoord, float yCoord);

    /**
     * Checks if the point matches the other point
     * @param other Other point to compare against
     * @return true if they are the same, false if they differ
     */
    bool operator==(const Point& other) const;

    /**
     * Checks if the point does not match the other point
     * @param other Other point to compare against
     * @return true if they differ, false if they are the same
     */
    bool operator!=(const Point& other) const;

    /**
     * Calculate the difference between this point and another
     * @param other Other point to compare against
     * @return Distance between the two points
     */
    float GetDistance(const Point& other) const;

    /// X coordinate of the point
    float x;

    /// Y coordinate of the point
    float y;
};

/**
 * Pair of points representing a line.
 */
class Line : public std::pair<Point, Point>
{
public:
    /**
     * Create a new line with both points at 0, 0
     */
    Line();

    /**
     * Create a new line with the specified points
     * @param a First point of the line
     * @param b Second point of the line
     */
    Line(const Point& a, const Point& b);

    /**
     * Create a new line with the specified point coordinates
     * @param aX X coordinate of the first point of the line
     * @param aY Y coordinate of the first point of the line
     * @param bX X coordinate of the second point of the line
     * @param bY Y coordinate of the second point of the line
     */
    Line(float aX, float aY, float bX, float bY);

    /**
     * Checks if the line matches the other line
     * @param other Other line to compare against
     * @return true if they are the same, false if they differ
     */
    bool operator==(const Line& other) const;

    /**
     * Determines if line intersects with the other line supplied
     * @param other Other line to compare against
     * @param point Output parameter to set where the intersection occurs
     * @param dist Output parameter to return the distance from the
     *  other line's first point to the intersection point
     * @return true if they intersect, false if they do not
     */
    bool Intersect(const Line& other, Point& point, float& dist) const;
};

/**
 * Represents a multi-point shape in a particular zone to be used
 * for calculating collisions. A shape can either be an enclosed
 * polygonal shape or a series of line segments.
 */
class ZoneShape
{
public:
    /**
     * Create a new shape
     */
    ZoneShape();

    /**
     * Determines if the supplied path collides with the shape
     * @param path Line representing a path
     * @param point Output parameter to set where the intersection occurs
     * @param surface Output parameter to return the first line to be
     *  intersected by the path
     * @return true if the line collides, false if it does not
     */
    bool Collides(const Line& path, Point& point,
        Line& surface) const;

    /// ID of the shape generated from a QMP file
    uint32_t ShapeID;

    /// Unique instance ID for the same shape ID from a QMP file
    uint32_t InstanceID;

    /// Name of the element representation from a QMP file
    libcomp::String ElementName;

    /// List of all lines that make up the shape. Since player movement
    /// uses arbitrary Z coordinates, these can be though of as surfaces
    std::list<Line> Surfaces;

    /// true if the shape is one or many line segments with no enclosure
    /// false if the shape is a solid enclosure
    bool IsLine;

    /// Represents the top left-most and bottom right most points of the
    /// shape. This is useful in determining if a shape could be collided
    /// with instead of checking each surface individually
    std::array<Point, 2> Boundaries;
};

/**
 * Represents all zone geometry retrieved from a QMP file for use in
 * calculating collisions
 */
class ZoneGeometry
{
public:
    /**
     * Determines if the supplied path collides with any shape
     * @param path Line representing a path
     * @param point Output parameter to set where the intersection occurs
     * @param surface Output parameter to return the first line to be
     *  intersected by the path
     * @param shape Output parameter to return the first shape the path
     *  will collide with. This will always be the shape the surface
     *  belongs to
     * @return true if the line collides, false if it does not
     */
    bool Collides(const Line& path, Point& point,
        Line& surface, std::shared_ptr<ZoneShape>& shape) const;

    /**
     * Determines if the supplied path collides with any shape
     * @param path Line representing a path
     * @param point Output parameter to set where the intersection occurs
     * @return true if the line collides, false if it does not
     */
    bool Collides(const Line& path, Point& point) const;

    /// QMP filename where the geometry was loaded from
    libcomp::String QmpFilename;

    /// List of all shapes
    std::list<std::shared_ptr<ZoneShape>> Shapes;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_ZONEGEOMETRY_H
