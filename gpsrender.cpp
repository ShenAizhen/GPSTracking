#include "gpsrender.h"
#include "polyline2d/include/Polyline2D.h"
#include <glm/gtx/vector_angle.hpp>
#include <QFile>

std::vector<glm::vec4> cPoints;

GPSRenderer::GPSRenderer(){
    GLfunc = nullptr;
    program = nullptr;
    programPath = nullptr;
    programPath2 = nullptr;
    handledLoad = false;
    runningES = true; //assume it is ES unless told otherwise
    viewportSize  = QSize(0, 0);
    viewportPoint = QPoint(0, 0);
    pGeometry = Graphics::new_empty_simple_geometry();

    pGrid.plane = Graphics::plane_new(glm::vec3(0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0),
                                      400.0f, 3);
//                                      20.0f, 15);
    pGrid.initialize_for(3, pGrid.plane->sideL);

    target = Graphics::target_new(3.0f, 0.2f);

    /* Use only 2D unless you want to manually sync zoom */
    view_system = new View(QVector3D(0, 40, -60),
                           target->graphicalLocation,
                           QVector3D(0, 1, 0),
                           Follow2D, Fixed,
                           viewportSize.width(),
                           viewportSize.height());

    view_system->view_follow(&target);
    cache_shaders();
}

void GPSRenderer::swap_view_mode(){
    view_system->swap_mode();
}

void GPSRenderer::setVisible(bool _visible){
    visible = _visible;
}

void GPSRenderer::clear_shaders(){
    if(program)     delete program;
    if(programPath) delete programPath;
    if(programPath2) delete programPath2;
}

GPSRenderer::~GPSRenderer(){
    clear_shaders();
}

void GPSRenderer::setViewportSize(const QSize &size){
    viewportSize = size;
}

void GPSRenderer::setViewportPoint(const QPoint &point){
    viewportPoint = point;
}

void GPSRenderer::cache_shaders(){
    QOpenGLContext *glCtx = QOpenGLContext::currentContext();
    runningES             = glCtx->isOpenGLES();
    cachedSegVertex       = load_versioned_shader(":/shaders/segment_vertex.glsl");
    cachedSegFrag         = load_versioned_shader(":/shaders/segment_frag.glsl");
    cachedInstVertex      = load_versioned_shader(":/shaders/instances_vertex.glsl");
    cachedInstFrag        = load_versioned_shader(":/shaders/instances_frag.glsl");
    cachedPathingVertex   = load_versioned_shader(":/shaders/pathing_vertex.glsl");
    cachedPathingFrag     = load_versioned_shader(":/shaders/pathing_frag.glsl");
    cachedPathing2Vertex  = load_versioned_shader(":/shaders/pathing2_vertex.glsl");
    cachedPathing2Frag    = load_versioned_shader(":/shaders/pathing2_frag.glsl");
}

void GPSRenderer::setShaders(){
    clear_shaders();

    program      = new QOpenGLShaderProgram();
    programPath  = new QOpenGLShaderProgram();
    programPath2 = new QOpenGLShaderProgram();

    programPath->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                         cachedPathingVertex);

    programPath->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                         cachedPathingFrag);

    programPath2->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                          cachedPathing2Vertex);

    programPath2->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          cachedPathing2Frag);

    program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                     cachedSegVertex);

    program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                     cachedSegFrag);

    GraphicsDebugger::debuggerShader = programPath;
}

QString GPSRenderer::load_versioned_shader(QString path){
    QString str;
    QFile file(path);
    if(file.open(QFile::ReadOnly)){
        QTextStream ss(&file);
        QString code = ss.readAll();
        if(runningES){
            str = "#version 300 es\n"
                  "precision mediump float;\n";
        }else{
            str = "#version 330\n";
        }

        str += code;
    }
    return str;
}

void GPSRenderer::assure_gl_functions(){
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if(!ctx){
        qDebug() << "No QOpenGLContext found!";
        exit(0);
    }

    if(!ctx->isValid()){
        qDebug() << "Invalid QOpenGLContext!";
        exit(0);
    }

    if(!GLfunc){
        GLfunc = new OpenGLFunctions();
        QSurfaceFormat format = ctx->format();
        QPair<int, int> pair = format.version();
        QString str("Found Version: OpenGL ");
        str += QString::number(pair.first) + QString(".") + QString::number(pair.second);
        qDebug() << str;
    }

    GLfunc->functions = ctx->functions();
    GLfunc->extraFunctions = ctx->extraFunctions();

    if(!GLfunc->functions || !GLfunc->extraFunctions){
        qDebug() << "Failed to get OpenGL pointers!";
        exit(0);
    }
}

void GPSRenderer::init(){
    assure_gl_functions();
    this->initializeOpenGLFunctions();
    qDebug() << "Vendor " << (char *)glGetString(GL_VENDOR);

    setShaders();
}

void GPSRenderer::zoom_in(){
    view_system->view_zoom_in();
}

void GPSRenderer::zoom_out(){
    view_system->view_zoom_out();
}

void GPSRenderer::render_debug(GPSOptions *options){
    if(voxWorld){
        Voxel2D::Voxel *ptr = nullptr;
        Voxel2D::Voxel *center = voxWorld->centerVoxel;

        GraphicsDebugger::render_voxel_GL33(center, view_system,
                                            options->debugVoxelPrimaryColor,
                                            GLfunc);
        if(center){
            ptr = center->parent;
            if(ptr){
                if(ptr->childNN != center){
                    GraphicsDebugger::render_voxel_GL33(ptr->childNN, view_system,
                                                        options->debugVoxelNeighboorColor,
                                                        GLfunc);
                }

                if(ptr->childNP != center){
                    GraphicsDebugger::render_voxel_GL33(ptr->childNP, view_system,
                                                        options->debugVoxelNeighboorColor,
                                                        GLfunc);
                }

                if(ptr->childPP != center){
                    GraphicsDebugger::render_voxel_GL33(ptr->childPP, view_system,
                                                        options->debugVoxelNeighboorColor,
                                                        GLfunc);
                }

                if(ptr->childPN != center){
                    GraphicsDebugger::render_voxel_GL33(ptr->childPN, view_system,
                                                        options->debugVoxelNeighboorColor,
                                                        GLfunc);
                }
            }
        }

        if(options->renderControlPoints){
            if(cPoints.size() > 0){
                GraphicsDebugger::render_points_GL33(cPoints, view_system,
                                                     options->debugCPointsColor,
                                                     GLfunc);
            }
        }
    }
}

void GPSRenderer::render_voxel_triangles(Voxel2D::Voxel *vox, GPSOptions *options){
    if(vox){
        int triangleCount = (int)vox->trianglesVertex->size() / 3;
        int maxSize = MAX_TRIANGLES_PER_CALL * 3;
        if(likely(triangleCount < MAX_TRIANGLES_PER_CALL)){            
            pGeometry->data  = vox->trianglesVertex;
            pGeometry->extraEx = vox->trianglesMaskEx;

            Graphics::geometry_simple_bind_GL33(pGeometry, GLfunc);
            Graphics::path_render_GL33(pGeometry, programPath2,
                                       view_system, GLfunc, *options);
        }else{
            int it = 0;
            int len = (int)vox->trianglesVertex->size();
            std::vector<glm::vec3> data, extra;
            std::vector<glm::mat3> extraEx;//rev
            while(triangleCount > 0){
                int amount = it + maxSize > len ? len : it + maxSize;
                data.insert(data.end(), vox->trianglesVertex->begin() + it,
                            vox->trianglesVertex->begin() + amount);
                extraEx.insert(extraEx.end(), vox->trianglesMaskEx->begin() + it,
                            vox->trianglesMaskEx->begin() + amount);

                qDebug() << "Rendering " << it << " => " << amount;
                it = amount;
                pGeometry->data = &data;
                pGeometry->extraEx = &extraEx;
                Graphics::geometry_simple_bind_GL33(pGeometry, GLfunc);
                Graphics::path_render_GL33(pGeometry, programPath2,
                                           view_system, GLfunc, *options);
                data.clear();
                extraEx.clear();
                triangleCount -= MAX_TRIANGLES_PER_CALL;
            }
        }
    }
}

void GPSRenderer::gl_render_scene(){
    /**
     * Render Pipeline:
     *      1 - Render the plane (floor) this must be the first step since we are
     *          unnable to disable the blend equation because of QML renderer;
     *
     *      2 - Render paths, make sure the color buffer alpha values in the
     *          blend equations becomes zero with GL_ZERO, this will erase
     *          previous colors. Disable the depth test and let the triangles
     *          intersect each other, the blend equation will render only
     *          the most recent which will be correct;
     *
     *      3 - Restore the blend equation to additive with GL_ONE, GL_ONE
     *          also enable again the depth test with GL_DEPTH_TEST this will
     *          make our target render on top of everything and we are done!
     */
    GPSOptions options = Metrics::get_gps_option();

    GLfunc->functions->glEnable(GL_DEPTH_TEST);
    GLfunc->functions->glDepthFunc(GL_LEQUAL);
    if(!pGrid.plane->geometry->is_binded){
        Graphics::geometry_bind_GL33(pGrid.plane->geometry, GLfunc);
    }

    Graphics::plane_grid_render_GL33(&pGrid, program, view_system,GLfunc, options);

    GLfunc->functions->glEnable(GL_BLEND);
    GLfunc->functions->glBlendFunc(GL_ONE, GL_ZERO);
    GLfunc->functions->glBlendEquation(GL_FUNC_ADD);

    view_system->compute_vp_matrix();
    pGeometry->data = nullptr;

    if(voxWorld){
        voxWorld->lock_voxels();
        Voxel2D::Voxel *vox = voxWorld->voxelListHead;
        int done = vox ? 0 : 1;
        while(!done){
            bool renderVoxel = vox->containerVoxel !=
                               voxWorld->centerVoxel->containerVoxel;
            if(renderVoxel){ // center voxel is delayed
                renderVoxel = view_system->is_voxel_visible(vox);
            }

            if(renderVoxel){
                render_voxel_triangles(vox, &options);
                if(options.enableDebugVars){
                    GraphicsDebugger::render_voxel_GL33(vox,
                                                        view_system,
                                                        options.debugVoxelContainerColor,
                                                        GLfunc);
                }
            }else if(vox->containerVoxel !=
                     voxWorld->centerVoxel->containerVoxel)
            {
                // this should never be seen, if you do... we have a bug on the way voxels
                // are being clipped by the view system
                if(options.enableDebugVars){
                    GraphicsDebugger::render_voxel_GL33(vox,
                                                        view_system,
                                                        options.debugVoxelError,
                                                        GLfunc);
                }
            }

            vox = vox->listNext;
            done = vox ? 0 : 1;
        }

        // render the center voxel on top of everything
        if(voxWorld->centerVoxel){
            vox = voxWorld->centerVoxel->containerVoxel;
            render_voxel_triangles(vox, &options);
        }

        voxWorld->unlock_voxels();
    }

    GLfunc->functions->glBlendFunc(GL_ONE, GL_ONE);
    GLfunc->functions->glBlendEquation(GL_FUNC_ADD);
    GLfunc->functions->glDisable(GL_BLEND);

    if(!target->geometry->is_binded){
        Graphics::geometry_bind_GL33(target->geometry, GLfunc);
    }

    Graphics::target_render_GL33(target, programPath, view_system,
                                 GLfunc, options);

    if(options.enableDebugVars){
        render_debug(&options);
    }
}

void GPSRenderer::render(){
    if(visible){
        assure_gl_functions();

        if (!program) {
            init();
        }

        Orientation_t oLoc = Metrics::get_orientation_safe(&cPoints, 1);
        if(oLoc.changed){
            if(!handledLoad){
                bool isLoading = false;
                glm::vec3 last, curr;
                float exp_elevation = 0.0f;
                int rlv = Metrics::get_load_vectors(last, curr, isLoading, exp_elevation);
                if(!isLoading && rlv >= 0){ // done loading
                    float elevation = target->graphicalLocation.y();
                    float dy = elevation - exp_elevation;
                    if(dy <= Metrics::min_dY){
                        elevation += (dy + Metrics::min_dY);
                    }

                    QVector3D realPosition(oLoc.pos.x, elevation, oLoc.pos.z);

                    target->lastGraphicalLocation = QVector3D(last.x, elevation, last.z);
                    target->differentialAngle = target->graphicalRotation - oLoc.fromNorthAngle;
                    if(glm::abs(target->differentialAngle) > 20.0){
                        target->graphicalRotation = oLoc.fromNorthAngle;
                    }

                    target->graphicalLocation    = realPosition;
                    target->differentialAngle    = oLoc.dA;
                    target->differentialDistance = oLoc.distance;
                    pGrid.update_if_needed(glm::vec3(realPosition.x(), 0.0f, realPosition.z()));
                    handledLoad = true;
                }
            }else{
                float elevation = target->graphicalLocation.y();
                float dy = elevation - oLoc.minElevation;
                if(dy <= Metrics::min_dY){
                    elevation += (dy + Metrics::min_dY);
                }

                QVector3D realPosition(oLoc.pos.x, elevation, oLoc.pos.z);

                target->lastGraphicalLocation = target->graphicalLocation;
                target->differentialAngle = target->graphicalRotation - oLoc.fromNorthAngle;
                if(glm::abs(target->differentialAngle) > 20.0){
                    target->graphicalRotation = oLoc.fromNorthAngle;
                }

                target->graphicalLocation    = realPosition;
                target->differentialAngle    = oLoc.dA;
                target->differentialDistance = oLoc.distance;
                pGrid.update_if_needed(glm::vec3(realPosition.x(), 0.0f, realPosition.z()));
            }
        }

        int width = viewportSize.width();
        int height = viewportSize.height();

        view_system->frame_update();
        view_system->update_dimension(width, height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(viewportPoint.x(), viewportPoint.y(), width, height);
        gl_render_scene();
    }
}
