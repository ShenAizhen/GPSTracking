#ifndef VOXEL2D_H
#define VOXEL2D_H

#include <polyline2d/include/Vec2.h>
#include <vector>
#include <common.h>
#include <QDebug>
#include <QMutex>
#include <bits.h>

#define TRIANGLE_BASE_HEIGHT 0.1f
#define VOXEL_SEGMENT_PROP 1.0f
#define QUADTREE_LEN_SIZE 5120
//#define QUADTREE_LEN_SIZE 2560 // 2560^2 = 6553600 ~ 655,36 Hec
#define QUADTREE_CONTAINER_LEVEL 5 // use either 5 or 6 for the container level
#define MINIMAL_TRIANGLE_OFFSET 80

#define ABS(x) (x) < 0 ? -(x) : (x)
#define MAX2(x, y) (x) > (y) ? (x) : (y)
#define MAX3(x, y, z) MAX2(MAX2(x, y), z)

/*
    Explanation for this file:
        This contains a structure of a Quadtree where each leaf (last child) is a 2D voxel.
        Triangles are stored in a 2-scheme structure:
        1 - Child voxels (leaf) contains an array of start indexes that can find
            a triangle by invoking start + 0, start + 1, start + 2 on the actual
            container structure (containerVoxel).
        2 - The actual triangle store location is at a voxel that is 'QUADTREE_CONTAINER_LEVEL'
            after the root node.

        Take for example the scheme QUADTREE_CONTAINER_LEVEL=2 this means that every voxel
        that has a level higher than QUADTREE_CONTAINER_LEVEL will have an pointer access
        to the voxel with level QUADTREE_CONTAINER_LEVEL and this voxel will have actual data
        on the pointer trianglesVertex, all child voxels and parent voxels will have NULL on this
        pointer.

        Voxel Level = VL

        VL=0
         _______________________            By that division VL=3 will have start indexes
        |           |     |     |           and VL=2 will be a container voxel for VL=3. All
        |VL=1       |VL=2 |_____|           other voxels that have VL < 2 will be empty.
        |           |     |VL=3 |           But note that for a voxel to actually have data (start idx)
        |___________|_____|_____|           it needs to have length equal to the voxelBaseLength
        |           |           |           which represents the minimal voxel size, otherwise
        |           |           |           it will be empty and only provide a container pointer
        |___________|___________|           to VL=2.

        Voxels that actually have data, i.e.: VL=3, will also be added to a special list
        indexed by voxelListHead on the voxel world structure. This list is a fast way
        for the renderer to quickly loop all voxels that might need to be displayed and perform
        clip tests to check if they must be shown on screen or not.
        The Quadtree of voxels also is usefull for performing intersection tests to see
        if two (or more) paths are crossing each other without wasting time searching for
        triangles.

        The representation of the geometry is stored with 4 values per triangle vertex:
        1 - The x coordinate of the vertex;
        2 - The z coordinate of the vertex;
        3 - A Bitmask that represents the state of the vertex;
        4 - The elevation computed for the triangle.

        (1) and (2) are the regular positions, all triangles are assumed to be slightly
        elevated from the ground but in general on the plane XZ and all 3 vertex liying
        on the same plane.
        (3) gives a fast and cheap way to store what happens to the path at a specific
        triangle. In general this mask depends on the amount of segments configured and
        for each segment exists 3 bits that provide relevant information.
        Take for example a work of 3 paths (segments), a possible mask for a vertex
        could be:
                                    001100011 = 99
        Each 3 bits represents the state of a segment for that vertex, the first bit (MSB)
        signals if the vertex pertence to that segment, the second bit represents the
        state of the application when this triangle was built and the last one (LSB) represents
        if this triangle was built in the intention of having an intersection on this segment:

            1 - First segment  001: Vertex does not lie in this segment
                                    Application painted this segment
                                    This was a intersection

            2 - Second segment 100: Vertex lies on this segment
                                    Application painted this segment
                                    This was not a intersection

            3 - Third segment  011: Vertex does not lies on this segment
                                    Application did not paint this segment
                                    This was a intersection

        For every vertex there is only one bit identifying its segment. This is the
        base for the intersection test and the paint done on GLSL. For every fragment
        we interpolate the position and see where the interpolation lies on the triangle
        and with the state of the source vertex we can correctly compute if a fragment
        must be painted with intersection/painted normal/discarded.

        Because there could be problems with self-intersecting triangles each triangle
        receives a custom elevation during intersection test that gets stored in (4).
        This elevations is done so that GL_DEPTH_TEST will correctly render triangles
        on top of each other and image can be correct.
*/

struct uint2{
    unsigned int a, b;
};

struct vec5{
    float x, y, z, w, r;

    vec5(float _x, float _y, float _z, float _w, float _r){
        x = _x; y = _y; z = _z; w = _w;
        r = _r;
    }
};

class Voxel2D{
public:

    typedef struct voxel{
        struct voxel *parent, *listNext;
        struct voxel *containerVoxel;
        struct voxel *childPP, *childPN;
        struct voxel *childNP, *childNN;
        int canHoldData; // inform if this voxel can hold data or is a guiding voxel for quadtree
        std::vector<uint2> *triangleHash; // list of triangles start indices
        std::vector<glm::vec3> *trianglesVertex; // geometry triangles for fast draw calls
        std::vector<glm::mat3> *trianglesMaskEx; // geometry triangles masks for fast draw calls
        int voxelLevel; // how far we have to go from head to reach this voxel (head is level 0)
        Vec2 center; // graphical center position
        float l2; // half of voxels length 1D
        float l4; // one fourth of voxels length 1D
        float l; // voxel length 1D
        int insertFlag;
        unsigned int startFlagged;
        int inserted; // indicates if this voxel is part of the geometry list
        Vec2 p0, p1, p2, p3;

        ~voxel() {
            if (childNN)
                delete childNN;
            if (childPN)
                delete childPN;
            if (childNP)
                delete childNP;
            if (childPP)
                delete childPP;

            if (triangleHash)
                delete triangleHash;
            if (trianglesVertex)
                delete trianglesVertex;
            if (trianglesMaskEx)
                delete trianglesMaskEx;

            inserted = 0;
        }

        voxel(){ init(); }

        void init(){
            parent = nullptr;
            childPP = nullptr;
            childPN = nullptr;
            childNP = nullptr;
            childNN = nullptr;
            insertFlag = 0;
            voxelLevel = -1;
            inserted = 0;
            trianglesVertex = nullptr;
            trianglesMaskEx = nullptr;

            triangleHash = nullptr;
            containerVoxel = nullptr;
            l  = 0.0f;
            l2 = 0.0f;
            l4 = 0.0f;
            p0 = Vec2{0.0f, 0.0f};
            p1 = Vec2{0.0f, 0.0f};
            p2 = Vec2{0.0f, 0.0f};
            p3 = Vec2{0.0f, 0.0f};
            center = Vec2{0.0f, 0.0f};
        }

        void insert_flag_clear_brothers(){
            if(parent){
                if(parent->childNN){
                    parent->childNN->insertFlag = 0;
                }

                if(parent->childPP){
                    parent->childPP->insertFlag = 0;
                }

                if(parent->childNP){
                    parent->childNP->insertFlag = 0;
                }

                if(parent->childPN){
                    parent->childPN->insertFlag = 0;
                }
            }
        }

        void insert_flag_clear_container(){
            if(containerVoxel){
                containerVoxel->insertFlag = 0;
            }
        }

        /**
         * Find at which segment the point v belongs by point-point test with
         * the global control points, also returns the total segment count.
         * @param v The target point.
         * @param segmentCount The amount of segment based on the control points.
         */
        int get_location(Vec2 v, int &segmentCount){
            float minDist = 99999.9f;
            int t = 0;
            segmentCount = 0;
            for(glm::vec4 &cpoint : controlPoints){
                float d = Vec2Maths::distance(Vec2{cpoint.x, cpoint.z}, v);
                if(d < minDist){
                    t = SCAST(int, cpoint.w);
                    minDist = d;
                }

                if(cpoint.w > segmentCount) segmentCount = cpoint.w;
            }

            return t;
        }

        /**
         * Inserts a triangle in this voxel if this start point was not flagged before.
         * For insertions we can have multi-voxel insertion, meaning this voxel will be
         * invoked multiple times. Mark this triangle as ALREADY inserted, so that multiple
         * insertions don't happen.
         * @param v0 Triangle vertex.
         * @param v1 Triangle vertex.
         * @param v2 Triangle vertex.
         * @param hitMask The bit mask of the state of segments regarding to hit.
         * @param appMask The bit mask of the state of segments regarding on/off.
         * @param total The total triangle count at this moment.
         * @param elevation The wished triangle elvation. It should be determined by
         *        a previous intersection test in order to not cause problems.
         */
        void flagged_triangle_pushEx(Vec2 v0, Vec2 v1, Vec2 v2,
                                   unsigned char *hitMask,
                                   unsigned char *appMask,
                                   unsigned int total,
                                   float elevation)
        {
            if(insertFlag == 0){
                if(containerVoxel->insertFlag == 0){
                    containerVoxel->insertFlag = 1;
                    containerVoxel->startFlagged = SCAST(unsigned int, containerVoxel->trianglesVertex->size());
                    int segmentCount = 0;
                    int f0 = get_location(v0, segmentCount);
                    int f1 = get_location(v1, segmentCount);
                    int f2 = get_location(v2, segmentCount);
                    segmentCount += 1;


                    int h0,h1,h2,h3,m0,m1,m2,m3;
                    h0 = h1 = h2 = h3 = m0 = m1 = m2 = m3 = 0;

                    h0 = (int)BitHelper::bitExtracted((unsigned int)hitMask[2], 4, 0);
                    h0 = hitMask[0] | hitMask[1] << 8 | h0 << 16 ;
                    h1 = (int)BitHelper::bitExtracted((unsigned int)hitMask[2], 4, 4);
                    h1 |= hitMask[3] << 4 | hitMask[4] << 12 ;
                    h2 = (int)BitHelper::bitExtracted((unsigned int)hitMask[7], 4, 0);
                    h2 = hitMask[5] | hitMask[6] << 8 | h2 << 16 ;
                    h3 = (int)BitHelper::bitExtracted((unsigned int)hitMask[7], 4, 4);
                    h3 |= hitMask[8] << 4 | hitMask[9] << 12 ;

                    m0 = (int)BitHelper::bitExtracted((unsigned int)appMask[2], 4, 0);
                    m0 = appMask[0] | appMask[1] << 8 | m0 << 16 ;
                    m1 = (int)BitHelper::bitExtracted((unsigned int)appMask[2], 4, 4);
                    m1 |= appMask[3] << 4 | appMask[4] << 12 ;
                    m2 = (int)BitHelper::bitExtracted((unsigned int)appMask[7], 4, 0);
                    m2 = appMask[5] | appMask[6] << 8 | m2 << 16 ;
                    m3 = (int)BitHelper::bitExtracted((unsigned int)appMask[7], 4, 4);
                    m3 |= appMask[8] << 4 | appMask[9] << 12 ;

                    containerVoxel->trianglesVertex->push_back(glm::vec3(v0.x, elevation, v0.y));
                    containerVoxel->trianglesMaskEx->push_back(glm::mat3(h0,h1,h2,h3,f0,m0,m1,m2,m3));

                    containerVoxel->trianglesVertex->push_back(glm::vec3(v1.x, elevation, v1.y));
                    containerVoxel->trianglesMaskEx->push_back(glm::mat3(h0,h1,h2,h3,f1,m0,m1,m2,m3));

                    containerVoxel->trianglesVertex->push_back(glm::vec3(v2.x, elevation, v2.y));
                    containerVoxel->trianglesMaskEx->push_back(glm::mat3(h0,h1,h2,h3,f2,m0,m1,m2,m3));
                }
                uint2 u2;
                u2.a = containerVoxel->startFlagged;
                u2.b = total;
                triangleHash->push_back(u2);
                insertFlag = 1;
            }
        }

//        int find_segment_in_mask(unsigned int mask, int count){
//            for(int i = 0; i < count; i += 1){
//                int st = i;
//                if(BitHelper::single_bit_is_set(mask, st)){
//                    return i;
//                }
//            }

//            return -1;
//        }

        /**
         * Check the state of the segment to which the point P lies on the triangle
         * given by the states gt0, gt1, gt2.
         * @param gt0 Triangle state for vertex v0.
         * @param gt1 Triangle state for vertex v1.
         * @param gt2 Triangle state for vertex v2.
         * @param p Point P for which we must find state.
         * @param segCount The amount of segments present during triangle construction.
         * @return Returns 1 in case the segment found was 'rendered', 0 otherwise.
         */
        int get_triangle_segment_stateEx(vec5 gt0, vec5 gt1,
                                       vec5 gt2, unsigned char* mask, Vec2 p, int segCount)
        {
            // pick a line on the triangle gt0,gt1,gt2,
            // a priori this could not be resolved like this, however
            // because the triangle always fills an entire direction on the
            // segment the line will always be a good approximation of the
            // transversal segment because 2 points of the 3 will always
            // be very close and so they will be either on the first segment 0
            // or the last one total-1.

            /**
             * NOTE: This test is only correct because of our high density
             * triangle count. If you decide to reduce the resolution of triangles
             * this test needs to be improved on the line chosing scheme.
             * You need to get a line that is the *best* approximation of the
             * transversal segment of the path on that triangle.
             */
            Vec2 v1, v2;
            int segStart, segEnd;
            int gt0start = (int)gt0.z;
            int gt1start = (int)gt1.z;
            int gt2start = (int)gt2.z;

            if(gt0start != gt1start){
                v1 = Vec2{gt0.x, gt0.y};
                v2 = Vec2{gt1.x, gt1.y};
                segStart = gt0start;
                segEnd   = gt1start;
            }else{
                v1 = Vec2{gt0.x, gt0.y};
                v2 = Vec2{gt2.x, gt2.y};
                segStart = gt0start;
                segEnd   = gt2start;
            }

            // if we could not find, this triangle is broken and possibly
            // was not construct by our Quadtree/Voxel scheme. There is
            // no recover to this we might need to halt the application [?]
            if(segStart < 0 || segEnd < 0){
                qDebug() << "Error: could not find segment";
                return -1;
            }

            // now project the point P on the chosen line and check its offset
            // from the start of the line. Using that distance find the segment and
            // its state from the triple bit mask.
            Vec2 pl     = Vec2Maths::project_onto_line_fast(v1, v2, p);
            Vec2 v1pl   = Vec2Maths::direction(v1, pl);
            float v1pld = Vec2Maths::distance(pl, v1);

            if(segStart > segEnd){ // going on -axis, reverse it
                Vec2 vaux = v1;
                int taux = segStart;
                segStart = segEnd;
                v1 = v2;
                segEnd = taux;
                v2 = vaux;
                v1pld = Vec2Maths::distance(v1, pl);
            }

            float xkAcc = 0.0f;
            int value = 0;
            for(int i = segStart; i <= segEnd; i += 1){
                xkAcc += configSegments[i];
                Vec2 Ak = Vec2Maths::add(v1, Vec2Maths::multiply(v1pl, xkAcc));
                float d = Vec2Maths::distance(v1, Ak);
                if(d > v1pld){
                    value = i;
                    break;
                }
            }

            int index = value / 8;
            int num = value % 8;
            int ret = (mask[index] >> num) & 1U;
            return ret;
        }

        /**
         * Test if the point P intersects any triangle in this voxel.
         * @param p The target point P.
         * @param totalTriangles The triangle count at the current moment.
         * @param ok Flag returned indicating if this point intersected any triangle.
         * @param targetElevation Return the smallest elevation required for rendering with GL_DEPTH_TEST.
         * @param segCount The amount of segments being used.
         * @return Returns the mask of the most promising triangle, i.e.: if a underlying
         *         segment was rendered returns that mask before any other.
         */
        unsigned int triangle_point_intersection(Vec2 p, unsigned int totalTriangles, bool *ok,
                                                 float *targetElevation, int segCount)
        {
            unsigned int rv = 0;
            *ok = false;
            float minElevation = 0.1f;
            int seghit = 0;
            for(size_t i = 0; i < triangleHash->size(); i += 1){
                uint2 uid2 = triangleHash->at(i);
                unsigned int id = uid2.a;
                glm::vec3 gt0 = containerVoxel->trianglesVertex->at(id + 0);
                glm::vec3 gt1 = containerVoxel->trianglesVertex->at(id + 1);
                glm::vec3 gt2 = containerVoxel->trianglesVertex->at(id + 2);
                Vec2 p0{gt0.x, gt0.z};
                Vec2 p1{gt1.x, gt1.z};
                Vec2 p2{gt2.x, gt2.z};
                unsigned int dif = totalTriangles - uid2.b;
                if(Vec2Maths::triangleContains(p0, p1, p2, p) &&
                        dif > MINIMAL_TRIANGLE_OFFSET)
                {
                    float maxElevation = MAX3(gt0.y, gt1.y, gt2.y);
                    if(maxElevation > minElevation){
                        minElevation = maxElevation + MINIMAL_ELEVATION_OFFSET;
                    }

                    *ok = true;
                    if(rv == 0){
                        glm::mat3 gt30 = containerVoxel->trianglesMaskEx->at(id + 0);
                        glm::mat3 gt31 = containerVoxel->trianglesMaskEx->at(id + 1);
                        glm::mat3 gt32 = containerVoxel->trianglesMaskEx->at(id + 2);

                        //  I don' understand why configure triangle with only one point and three segment number
                        vec5 gt40(gt0.x, gt0.z, gt30[1][1], gt0.y, 0);
                        vec5 gt41(gt1.x, gt1.z, gt31[1][1], gt1.y, 0);
                        vec5 gt42(gt2.x, gt2.z, gt32[1][1], gt2.y, 0);

                        unsigned char mask[10]="";
                        mask[0] = BitHelper::bitExtracted((unsigned int)gt30[1][2],8,0);
                        mask[1] = BitHelper::bitExtracted((unsigned int)gt30[1][2],8,8);
                        mask[2] = BitHelper::bitExtracted((unsigned int)gt30[1][2],4,16);
                        mask[2] = mask[2] << 4 | BitHelper::bitExtracted((unsigned int)gt30[2][0],4,0);
                        mask[3] = BitHelper::bitExtracted((unsigned int)gt30[2][0],8,4);
                        mask[4] = BitHelper::bitExtracted((unsigned int)gt30[2][0],8,12);
                        mask[5] = BitHelper::bitExtracted((unsigned int)gt30[2][1],8,0);
                        mask[6] = BitHelper::bitExtracted((unsigned int)gt30[2][1],8,8);
                        mask[7] = BitHelper::bitExtracted((unsigned int)gt30[2][1],4,16);
                        mask[7] = mask[7] << 4 | BitHelper::bitExtracted((unsigned int)gt30[2][2],4,0);
                        mask[8] = BitHelper::bitExtracted((unsigned int)gt30[2][2],8,4);
                        mask[9] = BitHelper::bitExtracted((unsigned int)gt30[2][2],8,12);

                        QString ret = print_binary_mask(mask,segCount);
                        seghit = get_triangle_segment_stateEx(gt40, gt41, gt42, mask, p, segCount);
                        //***********************************************************************************************
                        rv = seghit < 0 ? 0 : seghit;
                    }
//                    break; // i'm not sure we can optmize this search
//                    if(rv == 0x00){ // it might be possible with this but elevation might fail
//                        break;
//                    }
                }
            }

            *targetElevation = minElevation;
            return rv;
        }

//        /**
//         * Checks if given Triangle P0,P1,P2 intersects the rectangle representation of this Voxel.
//         * @param p0 Triangle vertex P0.
//         * @param p1 Triangle vertex P1.
//         * @param p2 Triangle vertex P2.
//         * @return true in case it intersects, false otherwise.
//         */
//        bool intersectsTriangle(Vec2 t0, Vec2 t1, Vec2 t2){
//            if(Vec2Maths::triangleIntersects(t0, t1, t2, p0, p2, p3)){
//                return true;
//            }
//            return Vec2Maths::triangleIntersects(t0, t1, t2, p0, p1, p3);
//        }

        void set_center(Vec2 c, float len, bool canHold=false){
            center = c;
            l  = len;
            l2 = len/2.0f;
            l4 = len/4.0f;
            p0 = Vec2Maths::add(center, Vec2{l2,  l2});
            p1 = Vec2Maths::add(center, Vec2{-l2, -l2});
            p2 = Vec2Maths::add(center, Vec2{-l2, l2});
            p3 = Vec2Maths::add(center, Vec2{l2,  -l2});
            set_hold(canHold);
        }

        void set_level(int level){
            voxelLevel = level;
        }

        void set_hold(bool canHold){
            canHoldData = canHold;
            if(canHoldData && !triangleHash){
                triangleHash = new std::vector<uint2>();
            }
        }

        void set_quadtree_center(Vec2 c, float l){
            set_center(c, l);
        }

        /**
         * Test if a given point lies inside this voxel boundaries.
         * @param point Point to be tested.
         * @return true if the point lies inside, false otherwise.
         */
        bool is_inside(Vec2 point){
            /**
             * I experimented with triangle area sum and projection
             * estimation. In the end the old 4 if conditions is better
             * for axis aligned planes.
             */
            bool pxbeqp1x = Vec2Maths::float_beq(point.x, p1.x); // point.x >= p1.x
            bool p2ybeqpy = Vec2Maths::float_beq(point.y, p3.y); // point.y >= p3.y
            return (pxbeqp1x && p0.x > point.x && p2ybeqpy && p2.y > point.y);
        }

        /**
         * Inserts a new triangle start in this voxel.
         * @param start The first position of the first vertex of the triangle
         *              in the container structure, must be a valid index
         *              from where operators + 0, + 1 and + 2 can locate all 3 vertices.
         */
        void push_triangle(unsigned int start, unsigned int total){
            uint2 u2;
            u2.a = start;
            u2.b = total;
            triangleHash->push_back(u2);
        }

        /**
         * @return The amount of triangles in this voxel
         */
        size_t triangle_count(){
            return triangleHash->size();
        }
    }Voxel;

    typedef struct voxel_world{
        QMutex insertLock;
        Voxel *voxelListHead, *voxelListTail;
        Voxel *centerVoxel; // Voxel that allways contains target object
        Voxel *quadTree; // First voxel header of the Quad-Tree
        int voxelsPerSide;
        int listTotalVoxels;
        int totalVoxels;
        float voxelBaseLength;
        Vec2 p0, p1, p2, p3;
        int createdVoxels;

        voxel_world(float base_length){
            voxelBaseLength = VOXEL_SEGMENT_PROP * base_length;
            quadTree = new Voxel();
            quadTree->set_quadtree_center(Vec2{0.0f, 0.0f}, QUADTREE_LEN_SIZE);
            quadTree->set_level(0);
            createdVoxels = 1;
            listTotalVoxels = 0;
            centerVoxel = nullptr;
            voxelListHead = nullptr;
            voxelListTail = nullptr;
        }

        ~voxel_world() {
            if (quadTree)
                delete quadTree;
        }

//        Vec2 hash_position_to_voxel(Vec2 point){
//            return Vec2{hash_1D(point.x,
//                                voxelBaseLength),
//                        hash_1D(point.y,
//                                voxelBaseLength)};
//        }

        void list_add_voxel(Voxel *voxel){
            if(!voxel->inserted){
                if(voxelListTail){
                    voxelListTail->listNext = voxel;
                    voxelListTail = voxel;
                }else{
                    voxelListHead = voxel;
                    voxelListTail = voxel;
                }

                voxel->listNext = nullptr;
                voxel->inserted = 1;
                listTotalVoxels += 1;
            }
        }

        void lock_voxels(){
            insertLock.lock();
        }

        void unlock_voxels(){
            insertLock.unlock();
        }

        Voxel * quadtree_choose_child(Voxel *curr, Vec2 target, bool &found){
            bool px = Vec2Maths::float_beq(target.x, curr->center.x);
            bool py = Vec2Maths::float_beq(target.y, curr->center.y);
            Vec2 dir{-1.0f, -1.0f};
            Voxel **ptr = &curr->childNN; //-X -Y
            found = false;
            if(px && py){ // +X +Y
                ptr = &curr->childPP;
                dir = Vec2{1.0f, 1.0f};
            }else if(!px && py){ // -X +Y
                ptr = &curr->childNP;
                dir = Vec2{-1.0f, 1.0f};
            }else if(px && !py){ // +X -Y
                ptr = &curr->childPN;
                dir = Vec2{1.0f, -1.0f};
            }

            Vec2 childCenter{0.0f, 0.0f};
            if(!(*ptr)){
                return nullptr;
            }else{
                found = ((*ptr)->is_inside(target) &&
                         Vec2Maths::float_eq(curr->l2, voxelBaseLength));
                return *ptr;
            }
        }

        Voxel * quadtree_choose_or_make_child(Voxel *curr, Vec2 target, bool &found){
            bool px = Vec2Maths::float_beq(target.x, curr->center.x);
            bool py = Vec2Maths::float_beq(target.y, curr->center.y);
            Vec2 dir{-1.0f, -1.0f};
            Voxel **ptr = &curr->childNN; //-X -Y
            found = false;
            if(px && py){ // +X +Y
                ptr = &curr->childPP;
                dir = Vec2{1.0f, 1.0f};
            }else if(!px && py){ // -X +Y
                ptr = &curr->childNP;
                dir = Vec2{-1.0f, 1.0f};
            }else if(px && !py){ // +X -Y
                ptr = &curr->childPN;
                dir = Vec2{1.0f, -1.0f};
            }

            Vec2 childCenter{0.0f, 0.0f};
            if(!(*ptr)){
                dir.x *= curr->l4;
                dir.y *= curr->l4;
                childCenter = Vec2Maths::add(curr->center, dir);
                *ptr = new Voxel();
                (*ptr)->set_center(childCenter, curr->l2);

                found = ((*ptr)->is_inside(target) &&
                         Vec2Maths::float_eq(curr->l2, voxelBaseLength));

                (*ptr)->set_hold(found);
                (*ptr)->set_level(curr->voxelLevel + 1);

                if ( (*ptr)->voxelLevel == QUADTREE_CONTAINER_LEVEL )
                {
                    (*ptr)->containerVoxel = *ptr;
                    (*ptr)->trianglesVertex = new std::vector<glm::vec3>();
                    (*ptr)->trianglesMaskEx = new std::vector<glm::mat3>();
                }
                else if ( (*ptr)->voxelLevel > QUADTREE_CONTAINER_LEVEL )
                {
                    (*ptr)->containerVoxel = curr->containerVoxel;

                    if ( !(*ptr)->containerVoxel->trianglesVertex )
                    {
                        (*ptr)->containerVoxel->trianglesVertex = new std::vector<glm::vec3>();
                        (*ptr)->containerVoxel->trianglesMaskEx = new std::vector<glm::mat3>();
                    }
                }

                (*ptr)->parent = curr;
                createdVoxels += 1;
            }else{
                found = ((*ptr)->is_inside(target) &&
                         Vec2Maths::float_eq(curr->l2, voxelBaseLength));
            }
            return *ptr;
        }

        int intersectsAnything(Vec2 point, unsigned int total, unsigned int &oldState,
                               float &hitElevation, int segCount)
        {
            int rv = 0;
            Voxel *vox = quadtree_find_voxel(point);
            float expectedElevation = 0.1f;
            if(vox){
                bool any = false;
                oldState = vox->triangle_point_intersection(point, total, &any,
                                                            &expectedElevation,
                                                            segCount);
                rv = any ? 1 : 0;
            }
            hitElevation = MAX2(hitElevation, expectedElevation);
            return rv;
        }

        void quadtree_insert_triangleEx(Vec2 v0, Vec2 v1, Vec2 v2, unsigned char* hitMask,
                                      unsigned char *appMask, unsigned int totalTriangles,
                                      float elevation)
        {
            insertLock.lock();
            int multiVoxel = 0;
            Voxel *vox0 = quadtree_find_or_build(v0);
            Voxel *vox1 = quadtree_find_or_build(v1);
            Voxel *vox2 = quadtree_find_or_build(v2);

            vox0->insert_flag_clear_brothers();
            vox1->insert_flag_clear_brothers();
            vox2->insert_flag_clear_brothers();

            vox0->insert_flag_clear_container();
            vox0->flagged_triangle_pushEx(v0, v1, v2, hitMask, appMask,
                                        totalTriangles, elevation);
            list_add_voxel(vox0->containerVoxel);

            if(vox0 != vox1){
                if(vox0->containerVoxel != vox1->containerVoxel){
                    vox1->insert_flag_clear_container();
                }
                vox1->flagged_triangle_pushEx(v0, v1, v2, hitMask, appMask,
                                            totalTriangles, elevation);
                list_add_voxel(vox1->containerVoxel);
                multiVoxel = 1;
            }

            if(vox2 != vox1){
                if(vox2->containerVoxel != vox0->containerVoxel){
                    vox2->insert_flag_clear_container();
                }
                vox2->flagged_triangle_pushEx(v0, v1, v2, hitMask, appMask,
                                            totalTriangles, elevation);
                list_add_voxel(vox2->containerVoxel);
                multiVoxel = 1;
            }else if(vox2 != vox0){
                if(vox2->containerVoxel != vox1->containerVoxel){
                    vox2->insert_flag_clear_container();
                }
                vox2->flagged_triangle_pushEx(v0, v1, v2, hitMask, appMask,
                                            totalTriangles, elevation);
                list_add_voxel(vox2->containerVoxel);
                multiVoxel = 1;
            }

            insertLock.unlock();
        }

        Voxel * quadtree_find_voxel(Vec2 pos){
            Voxel *aux = quadTree;
            bool found = false;
            if(centerVoxel){
                if(centerVoxel->is_inside(pos)){
                    found = true;
                    aux = centerVoxel;
                }
            }

            while(!found){
                aux = quadtree_choose_child(aux, pos, found);
                if(!aux) break;
            }

            return aux;
        }

        Voxel * quadtree_find_or_build(Vec2 pos){
            Voxel *aux = quadTree;
            bool found = false;
            // first try the center voxel we might get lucky
            if(centerVoxel){
                if(centerVoxel->is_inside(pos)){
                    found = true;
                    aux = centerVoxel;
                }
            }

            int iterations = 0;

            while(!found){
                aux = quadtree_choose_or_make_child(aux, pos, found);
                iterations += 1;
                if(iterations > 10){
                    qDebug() << "Warning: Could not find proper voxel for point " << Vec2QStr(pos);
                    exit(0);
                }
            }
            return aux;
        }

        void update_center_voxel(Vec2 objPosition){
            centerVoxel = quadtree_find_or_build(objPosition);
        }

    }VoxelWorld;
};

#endif // VOXEL2D_H
