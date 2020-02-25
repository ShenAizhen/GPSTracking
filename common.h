#ifndef COMMON_H
#define COMMON_H
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <QVector>
#include <vector>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QGeoCoordinate>
#include <polyline2d/include/Vec2.h>

/**
 * Because we need to be able to index segments inside
 * a 32-bit integer for GLSL you can have at most 31
 * segments. Note however this would be very unstable. You
 * should use a more moderate number like < 6. Anything larger
 * than 10 might need to be optmize or reviwed for both
 * performance and accuracy.
 */
#define MAX_SEGMENTS               80
#define MAX_MASK_SEG               10 //  = 80 / 8

/**
 * This is only used for the path switching part of the renderer
 * it might be useless, I haven't checked. If you want to run this on
 * anything that is not GCC rewrite this define with something like:
 *  #define likely(x) (x)
 * and remove the 'optimization' provided by GCC.
 */
#define likely(x) __builtin_expect((x), 1)

/**
 * Zoom is currently broken in 3D because of 2D/3D level
 * synchronization. It needs work, disable this feature
 * for now. Only enable this if you are debugging something
 * and need a 3D perspective, but expect errors in zoom and
 * (because we are dynamically elevating triangles) the path generated
 * will be floating.
 */
#define ENABLE_3D_VIEW_SWITCH       1

/**
 * This is not a hard constraint, we try to achieve this but things might not happen.
 * You can change the TARGET_FPS value but never remove, all animations and transitions
 * will be based on this frame rate. This gives oportunity for the GPSViewer to update its
 * state to correctly interpolate through positions and matrices for camera movement,
 * target animation, and overall component updates.
 */
#define TARGET_FPS                  5

/**
 * Depending on the GPU we can increase this value
 * substantially, usually 15000 triangles is very little.
 * GPUs these days can render up to 200000 triangles in a single
 * render call or even more depending on the way it is rendering.
 * However we have no idea how the target hardware works, so I'm
 * beeing very gentle on the amount of triangles to be render
 * in one render call, note that this might cause the software
 * to invoke a lot of render calls which might slow down the
 * application because of data transfer to GPU, these are the kinda
 * of things that need to be optimized for the target hardware.
 */
#define MAX_TRIANGLES_PER_CALL    15000

/**
 * Our triangle scheme needs a constant elevation to preserve
 * the projection this is why 3D is not currently supported.
 * You can change this value but be carefull it might cause
 * the system to miss-behave on depth computations.
 */
#define MINIMAL_ELEVATION_OFFSET    0.05f

/**
 * Expose the control points for everyone, so that
 * Voxels can compute fast intersection and renderers
 * can debug properly.
 */
extern std::vector<glm::vec4> controlPoints;
extern GLfloat configSegments[MAX_SEGMENTS];
#define SCAST(type, val) static_cast<type>(val)
#define DCAST(type, val) dynamic_cast<type>(val)
#define Q3_MAKE_GLM3(val) glm::vec3((val).x(), (val).y(), (val).z())

#define BINARY6_PATTERN "%c%c%c%c%c%c"
#define BINARY8_PATTERN "%c%c%c%c%c%c%c%c"
#define BINARY12_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c"
#define BINARY18_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define BINARY20_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define BINARY32_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define TO_BINARY6(byte)\
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define TO_BINARY8(byte)\
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define TO_BINARY12(byte)\
    (byte & 0x800 ? '1' : '0'), \
    (byte & 0x400 ? '1' : '0'), \
    (byte & 0x200 ? '1' : '0'), \
    (byte & 0x100 ? '1' : '0'), \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

#define TO_BINARY18(byte)\
    (byte & 0x20000 ? '1' : '0'), \
    (byte & 0x10000 ? '1' : '0'), \
    (byte & 0x8000 ? '1' : '0'), \
    (byte & 0x4000 ? '1' : '0'), \
    (byte & 0x2000 ? '1' : '0'), \
    (byte & 0x1000 ? '1' : '0'), \
    (byte & 0x800 ? '1' : '0'), \
    (byte & 0x400 ? '1' : '0'), \
    (byte & 0x200 ? '1' : '0'), \
    (byte & 0x100 ? '1' : '0'), \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

#define TO_BINARY20(byte)\
    (byte & 0x80000 ? '1' : '0'), \
    (byte & 0x40000 ? '1' : '0'), \
    (byte & 0x20000 ? '1' : '0'), \
    (byte & 0x10000 ? '1' : '0'), \
    (byte & 0x8000 ? '1' : '0'), \
    (byte & 0x4000 ? '1' : '0'), \
    (byte & 0x2000 ? '1' : '0'), \
    (byte & 0x1000 ? '1' : '0'), \
    (byte & 0x800 ? '1' : '0'), \
    (byte & 0x400 ? '1' : '0'), \
    (byte & 0x200 ? '1' : '0'), \
    (byte & 0x100 ? '1' : '0'), \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

#define TO_BINARY32(byte)\
    (byte & 0x80000000 ? '1' : '0'), \
    (byte & 0x40000000 ? '1' : '0'), \
    (byte & 0x20000000 ? '1' : '0'), \
    (byte & 0x10000000 ? '1' : '0'), \
    (byte & 0x8000000 ? '1' : '0'), \
    (byte & 0x4000000 ? '1' : '0'), \
    (byte & 0x2000000 ? '1' : '0'), \
    (byte & 0x1000000 ? '1' : '0'), \
    (byte & 0x800000 ? '1' : '0'), \
    (byte & 0x400000 ? '1' : '0'), \
    (byte & 0x200000 ? '1' : '0'), \
    (byte & 0x100000 ? '1' : '0'), \
    (byte & 0x80000 ? '1' : '0'), \
    (byte & 0x40000 ? '1' : '0'), \
    (byte & 0x20000 ? '1' : '0'), \
    (byte & 0x10000 ? '1' : '0'), \
    (byte & 0x8000 ? '1' : '0'), \
    (byte & 0x4000 ? '1' : '0'), \
    (byte & 0x2000 ? '1' : '0'), \
    (byte & 0x1000 ? '1' : '0'), \
    (byte & 0x800 ? '1' : '0'), \
    (byte & 0x400 ? '1' : '0'), \
    (byte & 0x200 ? '1' : '0'), \
    (byte & 0x100 ? '1' : '0'), \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')
/**
 * Zoom levels available for 2D. Zooms are calculated
 * by projection compression, i.e.: for every level
 * get the projection of the box given by:
 *      Right  = -DEFAULT_2D_BOX_SIDE - offset;
 *      Left   =  DEFAULT_2D_BOX_SIDE + offset;
 *      Top    =  DEFAULT_2D_BOX_SIDE + offset;
 *      Bottom = -DEFAULT_2D_BOX_SIDE - offset;
 * Where offset is any of the defines bellow.
 * The values bellow have no specific meaning they are
 * simply values chosen to have an 'OK' display. Feel free
 * to change them. Note however that if the values flip, i.e.:
 * Right turns Left, the projection is flipped and you will see
 * the object in reverse, i.e.: under the floor. You can add
 * more levels by increasing the ZOOM_LEVELS, adding your target
 * FIXED_ZOOM_LEVEL_* and going to view.cpp and in Camera constructor
 * adding it to the zoom level array.
 */
#define ZOOM_LEVELS             5
#define FIXED_ZOOM_LEVEL_0    -20.0f
#define FIXED_ZOOM_LEVEL_1      0.0f
#define FIXED_ZOOM_LEVEL_2     40.0f
#define FIXED_ZOOM_LEVEL_3    120.0f
#define FIXED_ZOOM_LEVEL_4    200.0f

/**
 * These defines can be used for validation of the load part of the Database.
 * In order to test run things like this:
 *      1 - Delete any previous database file you might have;
 *      2 - Setup the GPSFakeProvider to use ACTIVE generation;
 *      3 - Uncomment the TEST_DB_GENERATE macro, and run the app
 *          it will write TEST_CHECK_POINT gps coordinates and exit;
 *      4 - Comment TEST_DB_GENERATE and uncoment TEST_DB_CONSUME macro
 *          and re-run the application. You should see the path is continued.
 */
#define TEST_CHECK_POINT        950
//#define TEST_DB_GENERATE
//#define TEST_DB_CONSUME

/*
 * The reason some of these are glm::vec3 and others are QVector3D is because
 * for geometry generation/manipulation glm is better but for rendering qt is better so we
 * swap data format to make our life easier.
*/

inline float hash_1D(float v, float baseLength){
    float c = std::abs(v);
    float ret = 0.0f;
    if(c > 0.5f * baseLength){
        float p = c - 0.5f * baseLength;
        int it = SCAST(int, p / baseLength);
        ret = (it + 1.0f) * baseLength;

        if(v < 0.0f){
            ret = -ret;
        }
    }

    return ret;
}

inline void print_binary(unsigned int mask){
    char msg[64] = {0};
    snprintf(msg, sizeof(msg), BINARY18_PATTERN, TO_BINARY18(mask));
    qDebug() << msg;
}

inline QString print_binary_mask(unsigned char *mask,int segCount)
{
    unsigned char maskEx[100]="";
    for(int i = 0 ; i < segCount ; ++i)
    {
        int index = i / 8;
        int num = i % 8;
        maskEx[i] = (mask[index] >> num) & 1U ;
    }

    QString str="";
    for(int i = 0; i < segCount ; i++ )
        str += QString::number( maskEx[i]);

    return str;
}

/**
 * Be very carefull when interacting with these structures
 * they are directly linked to OpenGL pipeline with the gl*
 * functions. A miss-configuration or invalid field might
 * generate a black screen and be very hard to debug.
 */
struct geometry_simple_t{
    std::vector<glm::vec3> *data;
    std::vector<glm::mat3> *extraEx;//rev
    GLuint vao, vbo[2];
    bool is_binded;
};

/* Representation of a geometry buffer needed to be render */
struct geometry_base_t{
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> nearPoints;
    std::vector<glm::vec3> farPoints;
    std::vector<glm::vec3> ptriangles;
    std::vector<glm::vec2> uvs;
    std::vector<ushort> triindices, quadindices;
    std::vector<glm::mat4> instancedModels;
    std::vector<QMatrix4x4> qInstancedModels;

    QMatrix4x4 modelMatrix;

    int triangles, quads;
    GLuint vbo, vao, triibo, quadibo;
    GLuint vboExtra;
    GLuint tmpvbos[4];
    bool is_binded;
    bool has_triindices;
    bool has_quadindices;
    int detail_level;
    bool instanced;
};

/* Geometric plane */
struct plane_t{
    struct geometry_base_t *geometry;
    glm::vec3 point;
    glm::vec3 normal;
    float sideL;
    glm::vec3 left, right;
};

struct plane_grid_t{
    struct plane_t *plane;
    int numLines, numCols;
    int nCount;
    float length;
    Vec2 p0, p1, p2, p3;
    Vec2 center;

    void initialize_for(int count, float sideL,
                        QVector3D qcenter = QVector3D(0.0f, 0.0f, 0.0f))
    {
        numLines = 2 * count + 1;
        numCols  = 2 * count + 1;
        nCount = count;
        length = sideL;
        float l2 = length/ 2.0f;
        center.x = qcenter.x();
        center.y = qcenter.y();
        plane->geometry->qInstancedModels.clear();
        p0 = Vec2Maths::add(center, Vec2{l2,  l2});
        p1 = Vec2Maths::add(center, Vec2{-l2, -l2});
        p2 = Vec2Maths::add(center, Vec2{-l2, l2});
        p3 = Vec2Maths::add(center, Vec2{l2,  -l2});

        for(int i = count; i >= -count; i -= 1){
            for(int j = -count; j <= count; j += 1){
                QMatrix4x4 model;
                float dx = j * sideL;
                float dz = i * sideL;
                model.translate(qcenter.x() + dx,
                                qcenter.y() + 0.0f,
                                qcenter.z() + dz);
                plane->geometry->qInstancedModels.push_back(model);
            }
        }
    }

    /**
     * I swear to god I saw this thing miss-calculate once but I'm unnable
     * to reproduce. Keep an eye out in case you have the feeling the object
     * is not aligned with the planes. All tests I make with small planes
     * gives perfect result, I have rewritten the four innequations multiple
     * times, it should be:
     *      1 - Px <= P3x or P0x
     *      2 - Px >= P1x or P2x
     *      3 - Pz >= P3y or P1y
     *      4 - Pz <= P2y or P0y
     * But the eyes don't lie ...
     */
    void update_if_needed(glm::vec3 point){
        bool pxbeqp1x = Vec2Maths::float_beq(point.x, p1.x); // point.x >= p1.x
        bool p2ybeqpy = Vec2Maths::float_beq(point.z, p3.y); // point.z >= p3.y
        if(!(pxbeqp1x && p0.x > point.x && p2ybeqpy && p2.y > point.z)){
            float x = hash_1D(point.x, length);
            float z = hash_1D(point.z, length);
            initialize_for(nCount, length,
                           QVector3D(x, 0.0f, z));
        }
    }

    int get_count(){
        return numLines * numCols;
    }

    QMatrix4x4 get_model(int i){
        QMatrix4x4 model;
        model.setToIdentity();
        bool hasAny = plane->geometry->qInstancedModels.size();
        if(i >= 0 && i < numLines * numCols && hasAny){
            model = plane->geometry->qInstancedModels[i];
        }
        return model;
    }

    QMatrix4x4 get_model(int i, int j){
        QMatrix4x4 model;
        model.setToIdentity();
        bool hasAny = plane->geometry->qInstancedModels.size();
        if(i >= 0 && j >= 0 && i < numLines && j < numCols && hasAny){
            int idx = i * numCols + j;
            model = plane->geometry->qInstancedModels[idx];
        }
        return model;
    }
};

/* Geometric target (arrow) */
struct target_t{
    struct geometry_base_t *geometry;
    float length;
    float baseHeight;
    QVector3D globalLocation;
    QVector3D graphicalLocation;
    QVector3D lastGraphicalLocation;
    QVector3D lastDirection, currentDirection;
    qreal graphicalRotation;
    qreal differentialAngle;
    qreal differentialDistance;
    glm::quat rotationQuaternion;
    int quaternionInited;
};

struct OpenGLFunctions{
    QOpenGLFunctions *functions;
    QOpenGLExtraFunctions *extraFunctions;
};

#endif // COMMON_H
