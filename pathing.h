#ifndef PATHING_H
#define PATHING_H
#include "common.h"
#include "view.h"
#include "polyline2d/include/Vec2.h"
#include "polyline2d/include/LineSegment.h"
#include <voxel2d.h>

namespace glm_extend{
    static float magnitude(const glm::vec2 &vec) {
        return std::sqrt(vec.x * vec.x + vec.y * vec.y);
    }

    static float cross(const glm::vec2 &a, const glm::vec2 &b){
        return a.x * b.y - a.y * b.x;
    }

    /**
     * Returns the positive angle that takes the rotation axis ab
     * from vec3 a to vec3 b
    */
    static float dp_angle(const glm::vec3 &a, const glm::vec3 &b){
        float    dot = a.x * b.x + a.y * b.y  + a.z * b.z;
        float lenSq1 = a.x * a.x + a.y * a.y + a.z * a.z;
        float lenSq2 = b.x * b.x + b.y * b.y + b.z * b.z;
        return std::acos(dot / std::sqrt(lenSq1 * lenSq2));
    }

    /**
     * Returns the counter-clockwise angle from vector a to b
    */
//    static float c_angle(const glm::vec2 &a, const glm::vec2 &b){
//        float dot = a.x * b.x + a.y * b.y;
//        float det = a.x * b.y - b.x * a.y;
//        return std::atan2(det, dot);
//    }

//    static float angle(const glm::vec2 &a, const glm::vec2 &b) {
//        return std::acos(dot(a, b) / (magnitude(a) * magnitude(b)));
//    }

//    static bool equal(const glm::vec2 &a, const glm::vec2 &b){
//        float eps = 0.00001f;
//        float diffa = a.x - b.x;
//        float diffb = a.y - b.y;
//        return (glm::abs(diffa) < eps && glm::abs(diffb) < eps);
//    }

//    static bool equal(const glm::vec3 &a, const glm::vec3 &b){
//        return equal(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z));
//    }
};

template<typename Vec2>
struct PolySegment {
    PolySegment(const LineSegment<Vec2> &center, float thickness) :
            center(center),
            // calculate the segment's outer edges by offsetting
            // the central line by the normal vector
            // multiplied with the thickness

            // center + center.normal() * thickness
            edge1(center + Vec2Maths::multiply(center.normal(), thickness)),
            edge2(center - Vec2Maths::multiply(center.normal(), thickness)) {}

    void set(const PolySegment<Vec2> &other){
        center = other.center;
        edge1 = other.edge1;
        edge2 = other.edge2;
    }

    LineSegment<Vec2> center, edge1, edge2;
};

class Pathing{

public:
    uchar currentHitMaskEx[MAX_MASK_SEG];
    uchar movementHitMaskEx[MAX_MASK_SEG];
    int lineCount;
    float thickness;
    unsigned int totalTriangles;
    struct geometry_base_t * dynamicGeometry;
    std::vector<PolySegment<Vec2>> segments;
    PolySegment<Vec2> *dynSeg, *lastSegment;
    Vec2 lastPoint;
    Vec2 lastPolyPoint;

    Vec2 nextStart1{0, 0};
    Vec2 nextStart2{0, 0};
    Vec2 start1{0, 0};
    Vec2 start2{0, 0};
    Vec2 end1{0, 0};
    Vec2 end2{0, 0};
    Vec2 vPoint{0.0f, 0.0f};

    Pathing(float section_size=0.0f);
    void set_segment_state(uchar *linesMask);
    void set_segment_state(int segment, int isOn);
    void set_segment_count(int segments);
    void clear_dynamic_triangles();
    void reset_state();
    void push_new_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2);
    void push_dynamic_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2);

};

#endif // PATHING_H
