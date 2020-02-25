#ifndef GPSRENDER_H
#define GPSRENDER_H

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#include "graphics.h"
#include "database.h"
#include <QTimer>

class GPSRenderer : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GPSRenderer();
    ~GPSRenderer();
    void setViewportSize(const QSize &size);
    void setViewportPoint(const QPoint &point);
    void setVisible(bool visible);
    void zoom_in();
    void zoom_out();
    void swap_view_mode();
    void gl_render_scene();
    void cache_shaders();
    void assure_gl_functions();
    void clear_shaders();
    QString load_versioned_shader(QString path);

public slots:
    void render();

private:
    void init();
    void setShaders();
    void render_debug(GPSOptions *options);
    void render_voxel_triangles(Voxel2D::Voxel *vox, GPSOptions *options);

private:
    QSize viewportSize;
    QPoint viewportPoint;
    QOpenGLShaderProgram *program;
    QOpenGLShaderProgram *programPath;
    QOpenGLShaderProgram *programPath2;
    struct OpenGLFunctions *GLfunc;
    View * view_system;
    bool visible;
    bool runningES;
    bool handledLoad;

    struct geometry_simple_t *pGeometry;
    struct plane_grid_t pGrid;
    struct target_t *target;

    QString cachedSegFrag, cachedSegVertex;
    QString cachedInstFrag, cachedInstVertex;
    QString cachedPathingVertex, cachedPathingFrag;
    QString cachedPathing2Vertex, cachedPathing2Frag;
};

#endif // GPSRENDER_H
