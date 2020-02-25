#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "common.h"
#include "view.h"
#include "pathing.h"
#include <gpsoptions.h>
#include <voxel2d.h>

/*
 * TODO[Must](Felipe): Agroup all painting/drawing properties inside a Themes class/struct
 * that should be uniform mappeed and work with that. Render calls already have a too big parameters list...
*/
extern Voxel2D::VoxelWorld *voxWorld;

typedef struct update_data_t{
   float x, z;
   uchar *currentHitMaskEx;
   uchar *movementMaskEx;
   int segments;
}update_data;

class Orientation_t{
public:
    QGeoCoordinate geoCoord;
    glm::vec3 realPos;
    glm::vec3 realDir;
    glm::vec3 dir;
    glm::vec3 prevDir;
    glm::vec3 pos;
    float distance;
    int valid;
    int changed;
    qreal fromNorthAngle;
    double dX, dY, dZ;
    double dA;
    int is_first;
    float minElevation;
    Orientation_t(){
        dir = glm::vec3(0,0,1);
        prevDir = glm::vec3(0,0,1);
        pos = glm::vec3(0,0,0);
        realPos = glm::vec3(0,0,0);
        valid = -1;
        distance = 0.0f;
        changed = 0;
        fromNorthAngle = -1.0;
        dX = 0.0; dY = 0.0; dZ = 1.0;
        dA = 0.0;
        is_first = 1;
    }
};

class Metrics{
public:
    static constexpr float min_dY = MINIMAL_ELEVATION_OFFSET;
    static constexpr qreal min_dD = 0.15;
    static constexpr qreal max_dXZ = 2.0;
    static constexpr qreal max_dA = 0.0872665;
//    static constexpr qreal max_dA = 0.349066;
    static constexpr qreal PI = 3.14159265359;
    static void initialize(GPSOptions options);
    static void initialize();
    static void finalize(GPSOptions options);
    static GPSOptions get_gps_option();
    static void update_gps_options(GPSOptions options);
    static QMatrix4x4 get_target_model_matrix(struct target_t *target);

    static QVector3D get_camera_object_vector(QVector3D graphicalLocation,
                                              QVector3D currentDirection,
                                              QVector3D source,
                                              float base_distance);

//    static unsigned char* get_current_move_mask_unsafe_ex(void);
    static void lock_for_series_update();
    static update_data update_orientation(QGeoCoordinate newLocation, int &changed);
    static void unlock_series_update();
//    static bool orientation_changed();
    static Orientation_t get_orientation_unsafe(/*std::vector<glm::vec4> *vec=nullptr*/);
    static Orientation_t get_orientation_safe(std::vector<glm::vec4> *vec=nullptr, int clean=0);
//    static void set_initial_orientation(Orientation_t orientation);
    static double azimuth_to_sphere(double azimuth);

//    static float get_graphical_rotation(qreal rotation);
    static float compute_xz_distance(QVector3D eye, QVector3D target);
    static void add_triangleEx(Vec2 v0, Vec2 v1, Vec2 v2, unsigned char* mask);
    static void set_line_state(uchar *linesMask);
    static void set_line_state(int line, int isOn);

    static void load_start();
    static void load_updateEx(QGeoCoordinate newLocation, unsigned char* moveMask);
    static void load_finish();
    static int get_load_vectors(glm::vec3 &last, glm::vec3 &curr,
                                bool &isLoading, float &elevation);

    static void setPath(Pathing *prePath);
    static void addPath(Pathing prePath);
};

/*
 * Generic class to provide basic OpenGL operations and
 * abstract geometry constuction/manipulation. Changed to use only
 * version 3.3+ since this should run with either OpenGL 3.3 or 3.0 ES
 * there is no point in providing OpenGL 2.1 compatibility it only makes
 * it harder to share shaders.
*/
class Graphics{
private:
    static void render_triangulation_GL33(struct geometry_base_t *geometry,
                                          QOpenGLShaderProgram *program,
                                          View *view_system, OpenGLFunctions *GLptr,
                                          QVector4D baseColor, QVector4D lineColor);

    static void render_triangulation_GL33(struct geometry_simple_t *geometry,
                                          QOpenGLShaderProgram *program,
                                          View *view_system, OpenGLFunctions *GLptr,
                                          QVector4D baseColor, QVector4D lineColor);

    static void render_indexed_triangulation_GL33(struct geometry_base_t *geometry,
                                                  QOpenGLShaderProgram *program,
                                                  View *view_system, OpenGLFunctions * GLptr,
                                                  QVector4D baseColor, QVector4D lineColor);
public:
//    static struct geometry_base_t   * new_empty_geometry();
    static struct geometry_simple_t * new_empty_simple_geometry();

    static struct plane_t * plane_new(glm::vec3 bpoint, glm::vec3 bnormal,
                                      float sidelen, int detail_level);

    static struct target_t * target_new(float length, float baseHeight);

    static void plane_grid_render_GL33(struct plane_grid_t *pGrid, QOpenGLShaderProgram *program,
                                       View *view_system, OpenGLFunctions * GLptr, GPSOptions options);

    static void target_render_GL33(struct target_t *target, QOpenGLShaderProgram *program,
                                   View *view_system, OpenGLFunctions * GLptr, GPSOptions options);

    static void path_render_GL33(struct geometry_simple_t *geometry, QOpenGLShaderProgram *program,
                                 View *view_system, OpenGLFunctions *GLptr, GPSOptions options);

    static void geometry_bind_GL33(struct geometry_base_t *geometry, OpenGLFunctions * GLptr,
                                   bool instanceSupport = false);

    static void geometry_simple_bind_GL33(struct geometry_simple_t *geometry,
                                          OpenGLFunctions *GLptr);
};

class GraphicsDebugger{
public:
    static QOpenGLShaderProgram *debuggerShader;
    static void bind_simple_geometry_GL33(struct geometry_base_t *geometry,
                                          OpenGLFunctions *GLptr);

    static void clear_simple_geometry_GL33(struct geometry_base_t *geometry,
                                           OpenGLFunctions *GLptr);

    static void render_points_GL33(std::vector<glm::vec4>points, View *view_system,
                                   QVector3D color, OpenGLFunctions * GLptr);

    static void render_voxel_GL33(Voxel2D::Voxel *vox, View *view_system,
                                  QVector3D color, OpenGLFunctions *GLptr);

//    static void render_triangles_GL33(struct geometry_base_t *geometry, View *view_system,
//                                      QVector3D start, QVector3D end, OpenGLFunctions *GLptr);

//    static void render_triangles_from_GL33(struct geometry_base_t *geometry, View *view_system,
//                                           QVector3D startCol, QVector3D endCol, int startIdx,
//                                           int endIdx, OpenGLFunctions *GLptr);

//    static void render_decomposed_GL33(std::vector<glm::vec3>points, View *view_system,
//                                       QVector3D start, QVector3D end, int hash,
//                                       OpenGLFunctions * GLptr);

    static void push_geometry(std::vector<glm::vec4> points, OpenGLFunctions *GLptr);
};

#endif // GRAPHICS_H
