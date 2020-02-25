#include "pathing.h"
#include "graphics.h"
#include "common.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <bits.h>

Pathing::Pathing(float section_size){
    QMatrix4x4 model; model.setToIdentity();
    memset(currentHitMaskEx,0,MAX_MASK_SEG);//rev
    memset(movementHitMaskEx,0,MAX_MASK_SEG);//rev
    this->lineCount = 0;
    this->thickness = section_size;
    lastSegment = nullptr;
    dynamicGeometry = new struct geometry_base_t;
    dynamicGeometry->instanced = false;
    dynamicGeometry->has_triindices = false;
    dynamicGeometry->has_quadindices = false;
    dynamicGeometry->triangles = 0;
    dynamicGeometry->quads = 0;
//    dynamicGeometry->modelMatrix = model;
    dynSeg = nullptr;
    lastPolyPoint.valid = 0;
    lastPoint = Vec2{0.0f, 0.0f};
    lastPoint.valid = 1;
    totalTriangles = 0;
}

void Pathing::set_segment_count(int segments){
    memset(movementHitMaskEx, 0, MAX_MASK_SEG);
    lineCount = segments;

    for(int i = 0; i < segments; ++i)
        BitHelper::single_bit_setEx(movementHitMaskEx, i);
}

void Pathing::set_segment_state(uchar *linesMask){
    memcpy(movementHitMaskEx, linesMask, MAX_MASK_SEG);
}

void Pathing::set_segment_state(int segment, int isOn)
{
    if (segment < lineCount && segment >= 0) {
        int is_set = BitHelper::single_bit_is_setEx(movementHitMaskEx, segment);

        if (isOn) {
            if (!is_set)
                BitHelper::single_bit_setEx(movementHitMaskEx,segment);
        } else {
            if (is_set)
                BitHelper::single_bit_clearEx(movementHitMaskEx,segment);
        }
    }
}

void Pathing::reset_state(){
    segments.clear();
    if(dynSeg) delete dynSeg;
    if(lastSegment) delete lastSegment;
    clear_dynamic_triangles();
    nextStart1  = Vec2{0, 0};
    nextStart2  = Vec2{0, 0};
    start1      = Vec2{0, 0};
    start2      = Vec2{0, 0};
    end1        = Vec2{0, 0};
    end2        = Vec2{0, 0};

    QMatrix4x4 model; model.setToIdentity();

    memset(currentHitMaskEx,0,MAX_MASK_SEG);//rev
    memset(movementHitMaskEx,0,MAX_MASK_SEG);//rev

    this->lineCount = 0;
    this->thickness = 0.0f;
    lastSegment = nullptr;
    dynamicGeometry = new struct geometry_base_t;
    dynamicGeometry->instanced = false;
    dynamicGeometry->has_triindices = false;
    dynamicGeometry->has_quadindices = false;
    dynamicGeometry->triangles = 0;
    dynamicGeometry->quads = 0;
    dynSeg = nullptr;
    lastPolyPoint.valid = 0;
    lastPoint = Vec2{0.0f, 0.0f};
    lastPoint.valid = 1;
    totalTriangles = 0;
}

void Pathing::clear_dynamic_triangles(){
    dynamicGeometry->positions.clear();
    dynamicGeometry->triangles = 0;
}

void Pathing::push_dynamic_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2){
    dynamicGeometry->positions.push_back(glm::vec3(v0.x, 0.101f, v0.y));
    dynamicGeometry->positions.push_back(glm::vec3(v1.x, 0.101f, v1.y));
    dynamicGeometry->positions.push_back(glm::vec3(v2.x, 0.101f, v2.y));
    dynamicGeometry->triangles += 1;
}

void Pathing::push_new_triangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2){
    totalTriangles += 1;
    Metrics::add_triangleEx(Vec2{v0.x, v0.y}, Vec2{v1.x, v1.y},
                            Vec2{v2.x, v2.y}, movementHitMaskEx);
}
