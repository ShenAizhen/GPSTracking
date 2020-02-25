#pragma once

#include <cmath>
#include <QDebug>
#define FLOAT_MIN_EPS 0.0000001f
#define Vec2QStr(a) "[" << (a).x << "," << (a).y << "]"

/**
 * A two-dimensional float vector.
 * It exposes the x and y fields
 * as required by Polyline2D/Voxel2D functions.
 */
struct Vec2 {
    Vec2() :
            Vec2(0, 0) {valid=1;}

    Vec2(float x, float y) :
            x(x), y(y) {valid=1;}

    virtual ~Vec2() = default;

    float x, y;
    int valid;
};

namespace Vec2Maths {

template<typename Vec2>
static bool equal(const Vec2 &a, const Vec2 &b) {
    return a.x == b.x && a.y == b.y;
}

template<typename Vec2>
static Vec2 multiply(const Vec2 &a, const Vec2 &b) {
    return {a.x * b.x, a.y * b.y};
}

template<typename Vec2>
static Vec2 multiply(const Vec2 &vec, float factor) {
    return {vec.x * factor, vec.y * factor};
}

template<typename Vec2>
static Vec2 divide(const Vec2 &vec, float factor) {
    return {vec.x / factor, vec.y / factor};
}

template<typename Vec2>
static Vec2 add(const Vec2 &a, const Vec2 &b) {
    return {a.x + b.x, a.y + b.y};
}

template<typename Vec2>
static Vec2 subtract(const Vec2 &a, const Vec2 &b) {
    return {a.x - b.x, a.y - b.y};
}

template<typename Vec2>
static float magnitude(const Vec2 &vec) {
    return std::sqrt(vec.x * vec.x + vec.y * vec.y);
}

template<typename Vec2>
static Vec2 withLength(const Vec2 &vec, float len) {
    auto mag = magnitude(vec);
    auto factor = mag / len;
    return divide(vec, factor);
}

template<typename Vec2>
static Vec2 normalized(const Vec2 &vec) {
    return withLength(vec, 1);
}

/**
 * Calculates the dot product of two vectors.
 */
template<typename Vec2>
static float dot(const Vec2 &a, const Vec2 &b) {
    return a.x * b.x + a.y * b.y;
}

/**
 * Calculates the cross product of two vectors.
 */
template<typename Vec2>
static float cross(const Vec2 &a, const Vec2 &b) {
    return a.x * b.y - a.y * b.x;
}

/**
 * Calculates direction from two vectors.
 */
template<typename Vec2>
static Vec2 direction(const Vec2 &a, const Vec2 &b) {
    Vec2 v{0, 0};
    v.x = b.x - a.x;
    v.y = b.y - a.y;
    return normalized(v);
}

/**
 * Calculates distance from two vectors.
 */
template<typename Vec2>
static float distance(const Vec2 &a, const Vec2 &b) {
    float dx = (b.x - a.x) * (b.x - a.x);
    float dy = (b.y - a.y) * (b.y - a.y);
    return std::sqrt(dx + dy);
}

/**
 * Calculates the angle between two vectors.
 */
template<typename Vec2>
static float angle(const Vec2 &a, const Vec2 &b) {
    return std::acos(dot(a, b) / (magnitude(a) * magnitude(b)));
}

/**
 * Checks if a point P is inside a triangle given by the points A, B, C, it does
 * not count edges.
 * @param Point A - triangle.
 * @param Point B - triangle.
 * @param Point C - triangle.
 * @param Point P - test point.
 * @return returns true in case point P lies inside triangle ABC, false otherwise.
 */
template<typename Vec2>
static bool triangleContains(Vec2 a, Vec2 b, Vec2 c, Vec2 p){
    float det = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    return  det * ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)) > 0 &&
            det * ((c.x - b.x) * (p.y - b.y) - (c.y - b.y) * (p.x - b.x)) > 0 &&
            det * ((a.x - c.x) * (p.y - c.y) - (a.y - c.y) * (p.x - c.x)) > 0;
}

template<typename Vec2>
static float cross(Vec2 ppa, Vec2 ppb, Vec2 ppc,
                   Vec2 b, Vec2 c, float normal)
{
    float bx = b.x;
    float by = b.y;
    float cyby = c.y - by;
    float cxbx = c.x - bx;
    Vec2 pa = ppa;
    Vec2 pb = ppb;
    Vec2 pc = ppc;
    return !(
      (((pa.x - bx) * cyby - (pa.y - by) * cxbx) * normal < 0) ||
      (((pb.x - bx) * cyby - (pb.y - by) * cxbx) * normal < 0) ||
      (((pc.x - bx) * cyby - (pc.y - by) * cxbx) * normal < 0));
}

/**
 * Fast triangle intersection. Test if triangle t0a,t0b,t0c
 * intersects triangle t1a,t1b,t1c.
 */
template<typename Vec2>
static bool triangleIntersects(Vec2 t0a, Vec2 t0b, Vec2 t0c,
                               Vec2 t1a, Vec2 t1b, Vec2 t1c)
{
    float normal0 = (t0b.x-t0a.x)*(t0c.y-t0a.y)-(t0b.y-t0a.y)*(t0c.x-t0a.x);
    float normal1 = (t1b.x-t1a.x)*(t1c.y-t1a.y)-(t1b.y-t1a.y)*(t1c.x-t1a.x);
    return !(cross(t1a, t1b, t1c, t0a, t0b, normal0) ||
             cross(t1a, t1b, t1c, t0b, t0c, normal0) ||
             cross(t1a, t1b, t1c, t0c, t0a, normal0) ||
             cross(t0a, t0b, t0c, t1a, t1b, normal1) ||
             cross(t0a, t0b, t0c, t1b, t1c, normal1) ||
             cross(t0a, t0b, t0c, t1c, t1a, normal1));
}

template<typename Vec2>
static bool on_segment(Vec2 p, Vec2 q, Vec2 r){
    return (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
            q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y));
}

template<typename Vec2>
static int orientation(Vec2 p, Vec2 q, Vec2 r){
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < FLOAT_MIN_EPS) return 0;  // colinear
    return (val > 0) ? 1 : -1; // clock or counterclock wise
}

/**
 * Checks if the finite line given by points (p1, q1) intersects
 * finite line given by points (p2, q2), cover all possible cases.
 */
template<typename Vec2>
static bool lineIntersects(Vec2 p1, Vec2 q1, Vec2 p2, Vec2 q2){
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    if (o1 != o2 && o3 != o4) return true;
    if (o1 == 0 && on_segment(p1, p2, q1)) return true;
    if (o2 == 0 && on_segment(p1, q2, q1)) return true;
    if (o3 == 0 && on_segment(p2, p1, q2)) return true;
    if (o4 == 0 && on_segment(p2, q1, q2)) return true;

    return false;
}

/**
 * Checks if the finite line given by points (p1, q1) intersects
 * finite line given by points (p2, q2), cover all possible cases.
 */
template<typename Vec2>
static bool lineIntersectsEx(Vec2 p1, Vec2 q1, Vec2 p2, Vec2 q2){
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    if (o1 == 0 && on_segment(p1, p2, q1)) return true;
    if (o2 == 0 && on_segment(p1, q2, q1)) return true;
    if (o3 == 0 && on_segment(p2, p1, q2)) return true;
    if (o4 == 0 && on_segment(p2, q1, q2)) return true;

    return false;
}

/**
 * Checks if a point P is inside a rectangle given by the points A, B, C and D.
 * Uses area algorithm.
 * @param Point A - rectangle.
 * @param Point B - rectangle.
 * @param Point C - rectangle.
 * @param Point D - rectangle.
 * @param Point P - test point.
 * @return returns true in case point P lies inside rectangle ABCD, false otherwise.
 */

template<typename Vec2>
static bool boxContains(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 p){
    int n = 0;
    Vec2 inf(1000000, 1000000);
    if (lineIntersects(a, b, p, inf)) n++;
    if (lineIntersects(b, c, p, inf)) n++;
    if (lineIntersects(c, d, p, inf)) n++;
    if (lineIntersects(d, a, p, inf)) n++;
    if (lineIntersectsEx(a, b, p, inf)) return true;
    if (lineIntersectsEx(b, c, p, inf)) return true;
    if (lineIntersectsEx(c, d, p, inf)) return true;
    if (lineIntersectsEx(d, a, p, inf)) return true;
    return n % 2;
}

template<typename Vec2>
static Vec2 normal(Vec2 dir){
    Vec2 uDir = normalized(dir);
    return Vec2{-uDir.y, uDir.x};
}

/**
 * Projects point P onto line given by points v1 and v2.
 * Fast implementation, approximates solution.
 */
template<typename Vec2>
static Vec2 project_onto_line_fast(Vec2 v1, Vec2 v2, Vec2 p){
    Vec2 e1 = {v2.x - v1.x, v2.y - v1.y};
    Vec2 e2 = {p.x - v1.x, p.y - v1.y};
    float vDp = Vec2Maths::dot(e1, e2);
    float len2 = e1.x * e1.x + e1.y * e1.y;
    return Vec2{(v1.x + (vDp * e1.x) / len2),
                (v1.y + (vDp * e1.y) / len2)};
}

/**
 * High-precision comparators for float/Vec2
 */
static bool float_eq(float val, float c){ // test if val == c
    return (std::abs(val - c) <= 0.00001f * std::min(std::abs(val), std::abs(c)));
}

static bool float_beq(float val, float c){ // test if val >= c
    return (std::abs(val - c) <= 0.00001f * std::min(std::abs(val), std::abs(c))) || val > c;
}

static bool vec2_eq(Vec2 a, Vec2 b){ // test if a == b
    return (float_eq(a.x, b.x) && float_eq(a.y, b.y));
}

} // namespace Vec2Maths
