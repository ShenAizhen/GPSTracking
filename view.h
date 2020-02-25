#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QDateTime>
#include <QVector>
#include <queue>
#include <QMutex>
#include "common.h"
#include <voxel2d.h>
#define OPERATION_ZOOM_IN             0
#define OPERATION_ZOOM_OUT            1
#define OPERATION_DURATION            100
#define FIXED_ZOOM_STEP               10.0f
#define DEFAULT_2D_BOX_SIDE           30.0f
#define DEFAULT_3D_BASE_Y_DISTANCE    50.0f
#define DEFAULT_2D_BASE_Y_DISTANCE    150.0f
#define DEFAULT_3D_BASE_SWAP_DISTANCE 100.0f

#define INVALID_ANGLE -9999.0f

typedef enum{
    Follow3D, Follow2D
}Viewtype;

typedef enum{
    Fixed=0, Cubic
}CameraType;

struct CamMovement{
    int type;
    QVector3D target_pos;
    QVector3D target_dir;
    double duration;
    qint64 dt;
    bool finished;
    QVector3D end_point;
    QVector3D start_point;
    int starting;
    float ref_zoom_step;
};

struct ViewAnimation{
    bool running;
    qint64 dt;
    qint64 duration;
    float velocity;
    float targetdx;
    float currentdx;
    int type;
    QVector4D projBoxStart;
};

struct Bounds2D{
    float right, left, top, bottom;
};

class Camera : public QObject
{
    Q_OBJECT
public:
    int hasGhostFollow;
    QVector3D ghostTarget;
    QVector3D eye;
    QVector3D target;
    QVector3D up;
    QVector3D velocity;
    QVector3D ghostDir;
    Viewtype type;
    std::queue<float> zoom_in_queue;
    std::queue<float> zoom_out_queue;
    CameraType cam_type;
    float zoomableLevels[5];
    int currentZoomLevel;

    QVector<struct CamMovement *>pendingMovements;

    qint64 delta_time, last_time;
    float distance;
    float base_3D_distance,  base_3D_y;
    explicit Camera(QVector3D eye, QVector3D target, QVector3D up,
                    CameraType _type=Fixed, QObject *parent = nullptr);

    QMatrix4x4 get_view_matrix();
    void compute_timing();
    void set_type(Viewtype _type);
    void camera_frame_update_elastic(struct target_t *object);
    void camera_frame_update_fixed(struct target_t *object);
    void camera_frame_update(struct target_t *object);
    void interpolate(struct CamMovement *transition);
    void compute_properties();
    void cancel_all_pending_movements();
    void update_base_swap_distance();

    void push_movement(struct CamMovement *movement);
    void align_to_object(struct target_t *object);
    void align_to_object_3D(struct target_t *object);
    void align_to_object_2D(struct target_t *object);
    void camera_sync_to_2D(struct target_t *object);
    void camera_sync_to_3D(struct target_t *object);


signals:

public slots:
};

class View{
public:
    Camera *camera;
    Viewtype type;
    float farplane;
    float nearplane;
    float fov;
    float width, height;
    struct target_t **followed;
    QMatrix4x4 cachedVPMatrix;
    ViewAnimation vAnimation2D;
    Bounds2D bounds2D;
    QMatrix4x4 projectionMatrix;

    View(QVector3D eye, QVector3D target, QVector3D up,
         Viewtype type, CameraType ctype, float width, float height, float fov=45.0f);

    void update_dimension(float width, float height);

    Camera * get_camera();
    QMatrix4x4 get_projection_matrix();
    QMatrix4x4 get_viewport_matrix();
    void update_2D_bounds_to_offset(float offset);
    void update_2D_bounds(float right, float left,
                          float top, float bottom);
    void view_follow(struct target_t **target);
    void view_zoom_out();
    void view_zoom_in();
    void frame_update();
    void swap_mode();
    void compute_vp_matrix();
    bool is_voxel_visible(Voxel2D::Voxel *voxel, bool use_cached_vp=true);
//    bool is_point_visible(glm::vec3 point, bool use_cached_vp=true);
};

#endif // CAMERA_H
