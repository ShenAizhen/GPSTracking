#include "view.h"
#include <qmath.h>
#include <glm/gtx/vector_angle.hpp>
#include "graphics.h"

Camera::Camera(QVector3D _eye, QVector3D _target, QVector3D _up,
               CameraType ctype, QObject *parent) : QObject(parent)
{
    eye = _eye;
    target = _target;
    up = _up;
    cam_type = ctype;
    velocity = QVector3D(0.0f, 0.0f, 0.0f);
    last_time = QDateTime::currentMSecsSinceEpoch();
    base_3D_distance = DEFAULT_3D_BASE_SWAP_DISTANCE;
    base_3D_y = DEFAULT_3D_BASE_Y_DISTANCE;
    hasGhostFollow = 0;
    currentZoomLevel = 1;
    zoomableLevels[0] = FIXED_ZOOM_LEVEL_0;
    zoomableLevels[1] = FIXED_ZOOM_LEVEL_1;
    zoomableLevels[2] = FIXED_ZOOM_LEVEL_2;
    zoomableLevels[3] = FIXED_ZOOM_LEVEL_3;
    zoomableLevels[4] = FIXED_ZOOM_LEVEL_4;
    compute_properties();
}

void Camera::update_base_swap_distance(){
    base_3D_distance = Metrics::compute_xz_distance(eye, target);
    base_3D_y = eye.y();
}

void Camera::compute_properties(){
    distance = Metrics::compute_xz_distance(eye, target);
}

QMatrix4x4 Camera::get_view_matrix(){
    QMatrix4x4 ret;
    ret.setToIdentity();
    ret.lookAt(eye, target, up);
    return ret;
}

/*
 * Basic interpolation for camera movement animation. All transitions
 * will be linear since interpolating on the sphere is harder (sorry :| )
 * p0 = source position
 * v0 = source velocity
 * p1 = destination position
 * v1 = destination velocity
 * duration = expected duration of movement
 * dt = time passed
 * returns true if finished, false otherwise
*/
template<typename T = QVector3D>
bool cubic_interpolation_fixed_duration(T *p0, T *v0,
                                        T p1, T v1,
                                        float duration, float dt)
{
    bool finished = false;
    if(dt > 0){
        if(duration < dt){
            *p0 = p1 + (dt - duration) * v1;
            *v0 = v1;
            finished = true;
        }else{
            float t = (dt / duration);
            float u = (1.0f - t);

            float c0 = 1*u*u*u;
            float c1 = 3*u*u*t;
            float c2 = 3*u*t*t;
            float c3 = 1*t*t*t;

            float dc0 = -3*u*u;
            float dc1 = -6*u*t + 3*u*u;
            float dc2 = 6*u*t - 3*t*t;
            float dc3 = 3*t*t;

            T b0 = *p0;
            T b1 = *p0 + (duration / 3.0f)* *v0;
            T b2 = p1  - (duration / 3.0f)* v1;
            T b3 = p1;

            *p0 = c0*b0 + c1*b1 + c2*b2 + c3*b3;
            *v0 = (dc0*b0 + dc1*b1 + dc2*b2 + dc3*b3) * (1.0f / duration);
        }
    }

    return finished;
}

void Camera::interpolate(struct CamMovement *transition){
    if(transition->duration > 0){
        float dur = static_cast<float>(transition->duration);
        float dt = static_cast<float>(transition->dt);
        transition->finished = cubic_interpolation_fixed_duration(&this->eye,
                                                                 &this->velocity,
                                                                 transition->end_point,
                                                                 QVector3D(0.0f, 0.0f, 0.0f),
                                                                 dur, dt);
        transition->duration -= transition->dt;
        compute_properties();
    }else{
        transition->duration = 0;
        transition->finished = true;
    }
}

void Camera::set_type(Viewtype _type){
    type = _type;
}

void Camera::align_to_object(struct target_t *object){
    if(type == Follow2D){
        align_to_object_2D(object);
    }else if(type == Follow3D){
        align_to_object_3D(object);
    }
}

void Camera::align_to_object_2D(struct target_t *object){
    eye.setX(object->graphicalLocation.x());
    eye.setZ(object->graphicalLocation.z());
    target = object->graphicalLocation;

    QVector4D dir4(1,0,0,0);
    QVector3D dir3;
    QMatrix4x4 rot;
    float angle = SCAST(float, Metrics::azimuth_to_sphere(object->graphicalRotation));

    rot.setToIdentity();
    rot.rotate(angle, QVector3D(0,-1,0));
    dir4 = rot * dir4;
    dir3 = dir4.toVector3D();
    dir3.normalize();
    up = dir3;
    up.normalize();
}

void Camera::align_to_object_3D(struct target_t *object){
    QVector3D oLoc = object->graphicalLocation;
    QVector3D dir  = object->currentDirection;
    float theta = glm::angle(glm::vec3(ghostDir.x(), ghostDir.y(), ghostDir.z()),
                             glm::vec3(dir.x(), dir.y(), dir.z()));
    float ctheta = glm::cos(theta);
    float dD = distance * ctheta;

    QVector3D ghost = ghostTarget + object->differentialDistance * ctheta * ghostDir;
    float dDistance = ghost.distanceToPoint(oLoc);

    /*TODO(Felipe): We are almost there, let the jiggle end! */
    if(object->differentialAngle < 2.0 * Metrics::max_dA &&
       hasGhostFollow != 0 && dDistance < Metrics::max_dXZ && false)
    {

        QVector3D eyeloc = Metrics::get_camera_object_vector(ghost, ghostDir,
                                                             eye, dD);
        target = ghost;
        eye = eyeloc;
        ghostTarget = ghost;
    }else{
        QVector3D eyeloc = Metrics::get_camera_object_vector(oLoc, dir,
                                                             eye, distance);
        target = object->graphicalLocation;
        eye = eyeloc;
        ghostTarget = target;
        ghostDir = dir;
        hasGhostFollow = 1;
        compute_properties();
    }
}

void Camera::cancel_all_pending_movements(){
    while(pendingMovements.size() > 0){
        struct CamMovement *ptr = pendingMovements.front();
        delete ptr;
        pendingMovements.remove(0);
    }
}

void Camera::camera_sync_to_2D(struct target_t *object){
    cancel_all_pending_movements();
    eye.setY(object->graphicalLocation.y() + DEFAULT_2D_BASE_Y_DISTANCE);
    align_to_object_2D(object);
}

void Camera::camera_sync_to_3D(struct target_t *object){
    up = QVector3D(0,1,0);
    cancel_all_pending_movements();
    QVector3D eyeloc = Metrics::get_camera_object_vector(object->graphicalLocation,
                                                         object->currentDirection,
                                                         eye, distance);
    float oldY = base_3D_y;
    target = object->graphicalLocation;
    ghostTarget = target;
    ghostDir = object->currentDirection;
    hasGhostFollow = 1;
    eyeloc.setY(oldY);
    eye = eyeloc;
    compute_properties();
    update_base_swap_distance();
}

void Camera::push_movement(struct CamMovement *movement){
    pendingMovements.push_front(movement);
}

void Camera::camera_frame_update(struct target_t *object){
    if(cam_type == Fixed){
        camera_frame_update_fixed(object);
    }else if(cam_type == Cubic){
        camera_frame_update_elastic(object);
    }else{
       /* Unknown camera type, defaulting to fixed */
       camera_frame_update_fixed(object);
    }
}

void Camera::camera_frame_update_fixed(struct target_t *object){
    if(pendingMovements.size() > 0){
        struct CamMovement *ptr = pendingMovements.front();
        QVector3D dir;
        switch(ptr->type){
            case OPERATION_ZOOM_IN: {
                dir = object->graphicalLocation - this->eye;
                if(eye.y() > 2.0f * FIXED_ZOOM_STEP){
                    float zoom_step = FIXED_ZOOM_STEP;
                    ptr->ref_zoom_step = zoom_step;
                    ptr->start_point = eye;
                    dir.normalize();
                    ptr->end_point = eye + dir * ptr->ref_zoom_step;
                    eye = ptr->end_point;
                    target = object->graphicalLocation;
                    ghostTarget = target;
                    compute_properties();
                }
                ptr->finished = true;
            } break;

            case OPERATION_ZOOM_OUT: {
                dir = this->eye - object->graphicalLocation;
                float zoom_step = FIXED_ZOOM_STEP;
                ptr->ref_zoom_step = zoom_step;
                ptr->start_point = eye;
                dir.normalize();
                ptr->end_point = eye + dir * ptr->ref_zoom_step;
                eye = ptr->end_point;
                target = object->graphicalLocation;
                ghostTarget = target;
                ptr->finished = true;
                compute_properties();
            } break;
            default:{}
        }

        if(ptr->finished){
            delete ptr;
            update_base_swap_distance();
            pendingMovements.remove(0);
            align_to_object(object);
        }
    }else{
        align_to_object(object);
    }
}

void Camera::compute_timing(){
    qint64 current_time = QDateTime::currentMSecsSinceEpoch();
    this->delta_time = current_time - this->last_time;
    last_time = current_time;
}

/*
 * Note(Felipe): There seems to be a weird behavior when queueing
 * the zoom step for dynamic interpolated animations. Because of that
 * we have to snap to the correct position after animation, can become
 * annoying if the object moves too fast. Since we already have a fixed
 * implementation without animation I'll keep that way and maybe later
 * if we have some time I'll revisit this code to fix this issue.
*/
void Camera::camera_frame_update_elastic(struct target_t *object){
    if(pendingMovements.size() > 0){
        struct CamMovement *ptr = pendingMovements.front();
        QVector3D dir;
        ptr->dt = this->delta_time;
        if(ptr->starting){
            ptr->starting = 0;
            ptr->duration = OPERATION_DURATION;
            switch(ptr->type){
                case OPERATION_ZOOM_IN: {
                    dir = object->graphicalLocation - this->eye;
                    float zoom_step = 0.6f * dir.length();
                    if(zoom_in_queue.size() > 0){
                        zoom_step = this->zoom_in_queue.front();
                        this->zoom_in_queue.pop();
                    }
                    this->zoom_out_queue.push(zoom_step);
                    ptr->ref_zoom_step = zoom_step;
                    ptr->start_point = eye;
                    dir.normalize();
                    ptr->end_point = eye + dir * ptr->ref_zoom_step;
                    this->velocity = QVector3D(0,0,0);
                } break;

                case OPERATION_ZOOM_OUT: {
                    dir = this->eye - object->graphicalLocation;
                    float zoom_step = 0.6f * dir.length();
                    if(zoom_out_queue.size() > 0){
                        zoom_step = this->zoom_out_queue.front();
                        this->zoom_out_queue.pop();
                    }
                    this->zoom_in_queue.push(zoom_step);
                    ptr->ref_zoom_step = zoom_step;
                    ptr->start_point = eye;
                    dir.normalize();
                    ptr->end_point = eye + dir * ptr->ref_zoom_step;
                    this->velocity = QVector3D(0,0,0);
                } break;
                default:{}
            }
        }

        interpolate(ptr);

        if(ptr->finished){
            update_base_swap_distance();
            delete ptr;
            pendingMovements.remove(0);
            align_to_object(object);
        }
    }else{
        align_to_object(object);
    }
}

/***********************************************************************/

View::View(QVector3D eye, QVector3D target, QVector3D up,
     Viewtype _type, CameraType ctype, float _width, float _height, float _fov)
{
    width = _width;
    height = _height;
    farplane = 1000.0f;
    nearplane = 0.1f;
    fov = _fov;
    type = _type;
    vAnimation2D.running = false;
    followed = nullptr;
    camera = new Camera(eye, target, up, ctype, nullptr);
    camera->set_type(type);

    float offset = camera->zoomableLevels[camera->currentZoomLevel];
    update_2D_bounds_to_offset(offset);
    if(type == Follow3D){
        float aspect = width / height;
        projectionMatrix.setToIdentity();
        projectionMatrix.perspective(fov, aspect, nearplane, farplane);
    }

    camera->update_base_swap_distance();
}

void View::update_2D_bounds_to_offset(float offset){
    update_2D_bounds(-DEFAULT_2D_BOX_SIDE - offset,
                      DEFAULT_2D_BOX_SIDE + offset,
                      DEFAULT_2D_BOX_SIDE + offset,
                     -DEFAULT_2D_BOX_SIDE - offset);
}

void View::update_2D_bounds(float _right, float _left,
                            float _top, float _bottom)
{
    bounds2D.right  = _right;
    bounds2D.left   = _left;
    bounds2D.bottom = _bottom;
    bounds2D.top    = _top;
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(bounds2D.right, bounds2D.left, bounds2D.bottom,
                           bounds2D.top, nearplane, farplane);
}

void View::update_dimension(float _width, float _height){
    width = _width;
    height = _height;
    if(type == Follow3D){
        float aspect = width / height;
        projectionMatrix.setToIdentity();
        projectionMatrix.perspective(fov, aspect, nearplane, farplane);
    }
}

Camera * View::get_camera(){
    return camera;
}

QMatrix4x4 View::get_viewport_matrix(){
    float w2 = width / 2.0f;
    float h2 = height / 2.0f;

    QMatrix4x4 ret(w2, 0.0f, 0.0f, 0.0f,
                   0.0f, h2, 0.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f,
                   w2, h2, 0.0f, 1.0f);
    return ret;
}

QMatrix4x4 View::get_projection_matrix(){
    return projectionMatrix;
}

void View::view_zoom_out(){
    if(camera->currentZoomLevel < ZOOM_LEVELS-1){
        if(type == Follow2D){ // this is solved in the projection matrix
            float loffset = camera->zoomableLevels[camera->currentZoomLevel];
            camera->currentZoomLevel += 1;
            float offset = camera->zoomableLevels[camera->currentZoomLevel];
            if(camera->cam_type == Cubic){
                if(!vAnimation2D.running){
                    vAnimation2D.running = true;
                    vAnimation2D.duration = OPERATION_DURATION;
                    vAnimation2D.dt = 0.0;
                    vAnimation2D.currentdx = loffset;
                    vAnimation2D.velocity = 0.0f;
                    vAnimation2D.targetdx = offset;
                    vAnimation2D.type = OPERATION_ZOOM_OUT;
                }
            }else{
                update_2D_bounds_to_offset(offset);
            }
        }else if(type == Follow3D){ // this is solved in view matrix
            struct CamMovement *movement = new struct CamMovement;
            camera->currentZoomLevel += 1;
            movement->type = OPERATION_ZOOM_OUT;
            movement->starting = 1;
            camera->push_movement(movement);
        }
    }
}

void View::view_zoom_in(){
    if(camera->currentZoomLevel > 0){
        if(type == Follow2D){
            float loffset = camera->zoomableLevels[camera->currentZoomLevel];
            camera->currentZoomLevel -= 1;
            float offset  = camera->zoomableLevels[camera->currentZoomLevel];
            if(camera->cam_type == Cubic){
                if(!vAnimation2D.running){
                    vAnimation2D.running = true;
                    vAnimation2D.duration = OPERATION_DURATION;
                    vAnimation2D.dt = 0.0;
                    vAnimation2D.currentdx = loffset;
                    vAnimation2D.velocity = 0.0f;
                    vAnimation2D.targetdx = offset;
                    vAnimation2D.type = OPERATION_ZOOM_IN;
                }
            }else{
                update_2D_bounds_to_offset(offset);
            }
        }else if(type == Follow3D){
            struct CamMovement *movement = new struct CamMovement;
            camera->currentZoomLevel -= 1;
            movement->type = OPERATION_ZOOM_IN;
            movement->starting = 1;
            camera->push_movement(movement);
        }
    }
}

void View::swap_mode(){
    if(type == Follow3D){
        type = Follow2D;
        camera->set_type(type);
        camera->camera_sync_to_2D(*followed);
        float offset = camera->zoomableLevels[camera->currentZoomLevel];
        update_2D_bounds_to_offset(offset);
    }else{
#if ENABLE_3D_VIEW_SWITCH
        type = Follow3D;
        camera->set_type(type);
        camera->camera_sync_to_3D(*followed);
#else
        qDebug() << "Warning: @3D viewing is disabled";
#endif
    }
}

void View::compute_vp_matrix(){
    QMatrix4x4 viewMatrix = camera->get_view_matrix();
    QMatrix4x4 projMatrix = get_projection_matrix();
    cachedVPMatrix = projMatrix * viewMatrix;
}

QVector4D to_ndc(QVector4D p){
    return QVector4D(p.x() / p.w(),
                     p.y() / p.w(),
                     p.z() / p.w(),
                     1.0);
}

bool is_point_ndc_inside(QVector4D p){
    float xc = p.x();
    float yc = p.y();
    float zc = p.z();
    bool isXInside = Vec2Maths::float_beq(xc, -1.0f) &&
                     Vec2Maths::float_beq(1.0, xc);

    bool isYInside = Vec2Maths::float_beq(yc, -1.0f) &&
                     Vec2Maths::float_beq(1.0f, yc);

    bool isZInside = Vec2Maths::float_beq(zc, -1.0f) &&
                     Vec2Maths::float_beq(1.0f, zc);

    return (isXInside && isYInside && isZInside);
}

bool is_line_ndc_inside(QVector4D pa0, QVector4D pa1){
    Vec2 p1{-1.0f, -1.0f};
    Vec2 q1{-1.0f, 1.0f}; // right
    if(Vec2Maths::lineIntersects(p1, q1,
                                 Vec2{pa0.x(), pa0.y()},
                                 Vec2{pa1.x(), pa1.y()}))
    {
        return true;
    }

    q1.x = 1.0f; q1.y = 1.0f; // top
    if(Vec2Maths::lineIntersects(p1, q1,
                                 Vec2{pa0.x(), pa0.y()},
                                 Vec2{pa1.x(), pa1.y()}))
    {
        return true;
    }

    p1.x = 1.0f; p1.y = -1.0f; // left
    if(Vec2Maths::lineIntersects(p1, q1,
                                 Vec2{pa0.x(), pa0.y()},
                                 Vec2{pa1.x(), pa1.y()}))
    {
        return true;
    }

    q1.x = -1.0f; q1.y = -1.0f; // bottom
    if(Vec2Maths::lineIntersects(p1, q1,
                                 Vec2{pa0.x(), pa0.y()},
                                 Vec2{pa1.x(), pa1.y()}))
    {
        return true;
    }

    return false;

}

bool View::is_voxel_visible(Voxel2D::Voxel *voxel, bool use_cached_vp){
    QVector4D pc0, pc1, pc2, pc3;
    QVector4D p0(voxel->p0.x, 0.0f, voxel->p0.y, 1.0);
    QVector4D p1(voxel->p1.x, 0.0f, voxel->p1.y, 1.0);
    QVector4D p2(voxel->p2.x, 0.0f, voxel->p2.y, 1.0);
    QVector4D p3(voxel->p3.x, 0.0f, voxel->p3.y, 1.0);

    QMatrix4x4 VP = cachedVPMatrix;
    if(!use_cached_vp){
        QMatrix4x4 viewMatrix = camera->get_view_matrix();
        QMatrix4x4 projMatrix = get_projection_matrix();
        VP = projMatrix * viewMatrix;
    }

    // if any point is visible we are done
    pc0 = to_ndc(VP * p0);
    if(is_point_ndc_inside(pc0)) return true;

    pc1 = to_ndc(VP * p1);
    if(is_point_ndc_inside(pc1)) return true;

    pc2 = to_ndc(VP * p2);
    if(is_point_ndc_inside(pc2)) return true;

    pc3 = to_ndc(VP * p3);
    if(is_point_ndc_inside(pc3)) return true;

    // need to check lines
    if(is_line_ndc_inside(pc0, pc3)){
        return true;
    }

    if(is_line_ndc_inside(pc0, pc2)){
        return true;
    }

    if(is_line_ndc_inside(pc2, pc1)){
        return true;
    }

    if(is_line_ndc_inside(pc1, pc3)){
        return true;
    }

    return false;
}

//bool View::is_point_visible(glm::vec3 point, bool use_cached_vp){
//    QVector4D transformed;
//    QVector4D  position(point.x, point.y, point.z, 1.0);
//    if(use_cached_vp){
//        transformed = cachedVPMatrix * position;
//    }else{
//        QMatrix4x4 viewMatrix = camera->get_view_matrix();
//        QMatrix4x4 projMatrix = get_projection_matrix();
//        transformed = projMatrix * viewMatrix * position;
//    }

//    float wc = transformed.w();
//    float xc = transformed.x() / wc;
//    float yc = transformed.y() / wc;
//    float zc = transformed.z() / wc;
//    bool isXInside = Vec2Maths::float_beq(xc, -1.0f) &&
//                     Vec2Maths::float_beq(1.0, xc);

//    bool isYInside = Vec2Maths::float_beq(yc, -1.0f) &&
//                     Vec2Maths::float_beq(1.0f, yc);

//    bool isZInside = Vec2Maths::float_beq(zc, -1.0f) &&
//                     Vec2Maths::float_beq(1.0f, zc);

//    return (isXInside && isYInside && isZInside);
//}

void View::view_follow(struct target_t **target){
    followed = target;
}

void View::frame_update(){
    camera->compute_timing();
    camera->camera_frame_update(*followed);
    if(vAnimation2D.running){
        vAnimation2D.dt = camera->delta_time;
        float dur = SCAST(float, vAnimation2D.duration);
        float dt  = SCAST(float, vAnimation2D.dt);
        bool done = cubic_interpolation_fixed_duration<float>(&vAnimation2D.currentdx,
                                                              &vAnimation2D.velocity,
                                                              vAnimation2D.targetdx,
                                                              0.0f, dur, dt);
        vAnimation2D.duration -= vAnimation2D.dt;
        update_2D_bounds_to_offset(vAnimation2D.currentdx);
        vAnimation2D.running = !done;
    }
}
