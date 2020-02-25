#pragma once

#include "LineSegment.h"
#include "pathing.h"
#include <vector>
#include <iterator>

/**
 * Re-write of the Polyline2D for better performance, C++11 and dynamic
 * generation of path. The previous version of Polyline2D had the ability
 * to wrap around a path and connect it's end point to start point.
 * This is a very sensitive interface for Polyline, be carefull when making
 * changes or implementing features.
*/

class Polyline2D {
public:
    /**
     * Inserts a point into a previous created path, updating dynamic and static geometry.
     * If the point deviates slightly from the path but it still falls inside the extended
     * previous segment it will be projected in the segment before insertion. This prevents
     * the number of triangles to become too big without a good reason.
     * @param Pathing structure used for container.
     * @param point The point to be inserted.
     * @param thickness The path's thickness.
     * @tparam Vec2 The vector type to use for the vertices.
     *              Must have public non-const float fields "x" and "y".
     *              Must have a two-args constructor taking x and y values.
     *              See Vec2 for a type that satisfies these requirements.
     */

    template<typename Vec2>
    static void insertOne(Pathing *pp, const Vec2 &point, float thickness)
    {
        thickness /= 2;
        if(!Vec2Maths::vec2_eq(pp->lastPoint, point)){
            if(!pp->lastSegment){
                LineSegment<Vec2> seg(Vec2{0.0f, 0.0f}, Vec2{0.0f, 0.0f});
                pp->lastSegment = new PolySegment<Vec2>(seg, 0);
            }

            if(pp->segments.size() > 0){
                pp->lastSegment->set(pp->segments[pp->segments.size() - 1]);
                LineSegment<Vec2> seg(pp->lastPoint, point);
                if(!pp->dynSeg){
                    pp->dynSeg = new PolySegment<Vec2>(seg, thickness);
                }else{
                    pp->dynSeg->set(PolySegment<Vec2>(seg, thickness));
                }

                pp->clear_dynamic_triangles();
                /*
                 * We are going to transform the lastPoint from dynamicGeometry
                 * to the static. There are 2 possibilities:
                 *  1 - The lastPoint lies inside the extended rectangle already
                 *      contained the polyGeometry and therefore we need to extend
                 *      the rectangle;
                 *  2 - The lastPoint does not lie inside the extended rectangle
                 *      therefore we need to create a joint and a new segment.
                */

                int len = SCAST(int, pp->segments.size());
                auto lastPolySegment = &(pp->segments[len-2]);

                Vec2 dir = lastPolySegment->center.direction();
                float d = Vec2Maths::distance(pp->lastPolyPoint, pp->lastPoint);

                Vec2 b0 = lastPolySegment->edge1.b;
                Vec2 b1 = lastPolySegment->edge2.b;
                Vec2 a0 = Vec2Maths::add(b0, Vec2Maths::multiply(dir, d));
                Vec2 a1 = Vec2Maths::add(b1, Vec2Maths::multiply(dir, d));

//                qDebug() << "Test Box: " << Vec2QStr(a0) << " " << Vec2QStr(b0)
//                         << " " << Vec2QStr(a1) << " " << Vec2QStr(b1)
//                         << " Point: " << Vec2QStr(pp->lastPoint);

                if(Vec2Maths::boxContains(a0, b0, a1, b1, pp->lastPoint) && false){
                    /* We can extend the rectangle for a new one */
                    int solved = 0;
                    Vec2 p0px = Vec2Maths::direction(pp->lastPolyPoint, pp->lastPoint);
                    float angle = Vec2Maths::angle(p0px, dir);
                    float dproj = d;
                    Vec2 target = pp->lastPoint;
                    if((angle >= 0 && angle <= pi2)){ // test if not NaN, i.e. vectors are not parallel
                        if(angle > 0.149066f){ // too much off-side, let it make a turn
                            pp->segments.pop_back();
                            std::vector<Vec2> data;
                            data.push_back(pp->lastPolyPoint);
                            data.push_back(pp->lastPoint);
                            data.push_back(point);
                            pp->lastPolyPoint = pp->lastPoint;
                            pp->lastPoint = point;
                            create<Vec2, std::vector<Vec2>>(pp, data, thickness * 2.0f);
                            solved = 1;
                        }else{ // simplify by rectangle extension
                            dproj = d * std::cos(angle);
                            target = Vec2Maths::add(pp->lastPolyPoint, Vec2Maths::multiply(dir, dproj));
                        }
                    }

//                    qDebug() << "Transformed " << Vec2QStr(pp->lastPoint) << " => " << Vec2QStr(target)
//                             << " PoPx " << Vec2QStr(p0px) << " dproj " << dproj << " angle " << angle
//                             << " Dir " << Vec2QStr(dir);
                }
                pp->segments.pop_back();
                std::vector<Vec2> data;
                data.push_back(pp->lastPolyPoint);
                data.push_back(pp->lastPoint);
                data.push_back(point);
                pp->lastPolyPoint = pp->lastPoint;
                pp->lastPoint = point;
                create<Vec2, std::vector<Vec2>>(pp, data, thickness * 2.0f);

            }else{
                /*
                 * This is Pathing structure was not created from a previous path,
                 * this is the first point to be inserted.
                */
//                qDebug() << "Creating first segment";
                std::vector<Vec2> targets;
//                qDebug() << Vec2QStr(pp->lastPoint);
//                qDebug() << Vec2QStr(point);
                targets.push_back(pp->lastPoint);
                targets.push_back(point);

                pp->lastPolyPoint = pp->lastPoint;
                pp->lastPoint = point;
                create<Vec2, std::vector<Vec2>>(pp, targets, thickness * 2.0f);
            }
        }
    }

	/**
	 * Creates a vector of vertices describing a solid path through the input points.
     * @param Pathing Container structure.
	 * @param points The points of the path.
     * @param thickness The path's thickness.
	 * @tparam Vec2 The vector type to use for the vertices.
	 *              Must have public non-const float fields "x" and "y".
	 *              Must have a two-args constructor taking x and y values.
     *              See Vec2 for a type that satisfies these requirements.
	 * @tparam InputCollection The collection type of the input points.
	 *                         Must contain elements of type Vec2.
	 *                         Must expose size() and operator[] functions.
	 */

    template<typename Vec2, typename InputCollection>
    static void create(Pathing *pp, const InputCollection &points,
                       float thickness)
    {
		// operate on half the thickness to make our lives easier
		thickness /= 2;
        int has_any = pp->segments.size() > 0 ? 1 : 0;
        size_t startIdx = 0;
        if(has_any){
            startIdx = pp->segments.size() - 1;
        }
        // create poly segments from the points
		for (size_t i = 0; i + 1 < points.size(); i++) {
			auto &point1 = points[i];
			auto &point2 = points[i + 1];

			// to avoid division-by-zero errors,
			// only create a line segment for non-identical points
            if (!Vec2Maths::vec2_eq(point1, point2)) {
                pp->segments.emplace_back(LineSegment<Vec2>(point1, point2), thickness);
			}
		}

        if (pp->segments.empty()) {
			// handle the case of insufficient input points
            return;
		}

		// calculate the path's global start and end points
        auto &firstSegment = pp->segments[0];
        auto &lastSegment = pp->segments[pp->segments.size() - 1];
        auto pathStart1 = firstSegment.edge1.a;
        auto pathStart2 = firstSegment.edge2.a;
        auto pathEnd1 = lastSegment.edge1.b;
        auto pathEnd2 = lastSegment.edge2.b;

        if(has_any){
            pathStart1 = pp->nextStart1;
            pathStart2 = pp->nextStart2;
        }

		// generate mesh data for path segments
        int final = 0;
        for (size_t i = has_any ? startIdx : 0; i < pp->segments.size(); i++) {
            auto &segment = pp->segments[i];

            // calculate start
            if (!has_any) {
                // this is the first segment
                pp->start1 = pathStart1;
                pp->start2 = pathStart2;
            }

            if(has_any && i == startIdx){
                pp->start1 = pathStart1;
                pp->start2 = pathStart2;
            }

            if (i + 1 == pp->segments.size()) {
				// this is the last segment
                pp->end1 = pathEnd1;
                pp->end2 = pathEnd2;
                final = 1;
            } else {
                createJoint(pp, segment, pp->segments[i+1],
                            pp->end1, pp->end2, pp->nextStart1, pp->nextStart2);
			}

			// emit vertices
            if(final){
                pp->push_dynamic_triangle(glm::vec2(pp->start1.x,pp->start1.y),
                                          glm::vec2(pp->start2.x, pp->start2.y),
                                          glm::vec2(pp->end1.x, pp->end1.y));

                pp->push_dynamic_triangle(glm::vec2(pp->end1.x, pp->end1.y),
                                          glm::vec2(pp->start2.x, pp->start2.y),
                                          glm::vec2(pp->end2.x, pp->end2.y));
            }else{
                pp->push_new_triangle(glm::vec2(pp->start1.x,pp->start1.y),
                                      glm::vec2(pp->start2.x, pp->start2.y),
                                      glm::vec2(pp->end1.x, pp->end1.y));

                pp->push_new_triangle(glm::vec2(pp->end1.x, pp->end1.y),
                                      glm::vec2(pp->start2.x, pp->start2.y),
                                      glm::vec2(pp->end2.x, pp->end2.y));

                pp->start1 = pp->nextStart1;
                pp->start2 = pp->nextStart2;

            }
		}
	}

private:
	static constexpr float pi = 3.14159265358979323846f;
    static constexpr float pi2 = 6.28318530718f;

	/**
	 * The minimum angle of a round joint's triangles.
	 */
    static constexpr float roundMinAngle = 0.174533f; // ~10 degrees

    template<typename Vec2>
    static void createJoint(Pathing *pp, PolySegment<Vec2> &segment1, PolySegment<Vec2> &segment2,
                            Vec2 &end1, Vec2 &end2,
                            Vec2 &nextStart1, Vec2 &nextStart2)
    {
		// calculate the angle between the two line segments
        Vec2 dir1 = segment1.center.direction();
        Vec2 dir2 = segment2.center.direction();

        float angle = Vec2Maths::angle(dir1, dir2);

		// wrap the angle around the 180° mark if it exceeds 90°
		// for minimum angle detection
        float wrappedAngle = angle;
		if (wrappedAngle > pi / 2) {
			wrappedAngle = pi - wrappedAngle;
		}
        // find out which are the inner edges for this joint
        float x1 = dir1.x;
        float x2 = dir2.x;
        float y1 = dir1.y;
        float y2 = dir2.y;

        bool clockwise = x1 * y2 - x2 * y1 < 0;

        const LineSegment<Vec2> *inner1, *inner2, *outer1, *outer2;

        // as the normal vector is rotated counter-clockwise,
        // the first edge lies to the left
        // from the central line's perspective,
        // and the second one to the right.
        if (clockwise) {
            outer1 = &segment1.edge1;
            outer2 = &segment2.edge1;
            inner1 = &segment1.edge2;
            inner2 = &segment2.edge2;
        } else {
            outer1 = &segment1.edge2;
            outer2 = &segment2.edge2;
            inner1 = &segment1.edge1;
            inner2 = &segment2.edge1;
        }

        // calculate the intersection point of the inner edges
        Vec2 innerSecOpt = LineSegment<Vec2>::intersection(*inner1, *inner2, false);

        Vec2 innerSec = innerSecOpt.valid
                        ? innerSecOpt
                        // for parallel lines, simply connect them directly
                        : inner1->b;

        // if there's no inner intersection, flip
        // the next start position for near-180° turns
        Vec2 innerStart;
        if (innerSecOpt.valid) {
            innerStart = innerSec;
        } else if (angle > pi / 2) {
            innerStart = outer1->b;
        } else {
            innerStart = inner1->b;
        }

        if (clockwise) {
            end1 = outer1->b;
            end2 = innerSec;

            nextStart1 = outer2->a;
            nextStart2 = innerStart;

        } else {
            end1 = innerSec;
            end2 = outer1->b;

            nextStart1 = innerStart;
            nextStart2 = outer2->a;
        }

        // draw a circle between the ends of the outer edges,
        // centered at the actual point
        // with half the line thickness as the radius
        createTriangleFan(pp, innerSec, segment1.center.b, outer1->b,
                          outer2->a, clockwise);
    }

	/**
	 * Creates a partial circle between two points.
	 * The points must be equally far away from the origin.
     * @param Pathing Container structure.
	 * @param connectTo The position to connect the triangles to.
	 * @param origin The circle's origin.
	 * @param start The circle's starting point.
	 * @param end The circle's ending point.
	 * @param clockwise Whether the circle's rotation is clockwise.
     * @param final Flag indicating if this should be inserted in dynamic or static geometry
	 */
    template<typename Vec2>
    static void createTriangleFan(Pathing *pp, Vec2 connectTo, Vec2 origin,
                                  Vec2 start, Vec2 end, bool clockwise)
    {
        Vec2 point1 = Vec2Maths::subtract(start, origin);
        Vec2 point2 = Vec2Maths::subtract(end, origin);

		// calculate the angle between the two points
        float angle1 = atan2(point1.y, point1.x);
        float angle2 = atan2(point2.y, point2.x);

		// ensure the outer angle is calculated
		if (clockwise) {
			if (angle2 > angle1) {
				angle2 = angle2 - 2 * pi;
			}
		} else {
			if (angle1 > angle2) {
				angle1 = angle1 - 2 * pi;
			}
		}

        float jointAngle = angle2 - angle1;

		// calculate the amount of triangles to use for the joint
        float numTriangles = std::max(1, SCAST(int, std::floor(std::abs(jointAngle) / roundMinAngle)));

		// calculate the angle of each triangle
        float triAngle = jointAngle / numTriangles;

		Vec2 startPoint = start;
		Vec2 endPoint;
		for (int t = 0; t < numTriangles; t++) {
			if (t + 1 == numTriangles) {
				// it's the last triangle - ensure it perfectly
				// connects to the next line
				endPoint = end;
			} else {
                float rot = (t + 1) * triAngle;

				// rotate the original point around the origin
				endPoint.x = std::cos(rot) * point1.x - std::sin(rot) * point1.y;
				endPoint.y = std::sin(rot) * point1.x + std::cos(rot) * point1.y;

				// re-add the rotation origin to the target point
				endPoint = Vec2Maths::add(endPoint, origin);
			}

            pp->push_new_triangle(glm::vec2(startPoint.x, startPoint.y),
                                  glm::vec2(endPoint.x, endPoint.y),
                                  glm::vec2(connectTo.x, connectTo.y));

			startPoint = endPoint;
		}
	}
};
