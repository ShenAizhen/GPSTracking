#include "graphics.h"
#include <QDebug>
#include <QString>
#include <qmath.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <QMutex>
#include "polyline2d/include/Polyline2D.h"
#include <bits.h>

#define GL_CHK(x, ptr) do{ __gl_clear_errors(ptr); ptr->extraFunctions->x; __gl_validate(#x, __LINE__, __FILE__, ptr);}while(0)
/****************************************************************************/
/*                 G R A P H I C S      F U N C T I O N S                   */
/****************************************************************************/

static glm::vec2 get_segment_and_length();

static std::string
translateGLError(int errorCode) {
    std::string error;
    switch (errorCode)
    {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        default: error = "Unknown Error";
    }
    return error;
}

void __gl_clear_errors(OpenGLFunctions *ptr) {
    while (ptr->functions->glGetError()) {
        ;
    }
}

void __gl_validate(const char *cmd, int line, const char *fileName,
                   OpenGLFunctions * ptr)
{
    (void)(fileName);
    (void)(line);
    GLenum val = ptr->functions->glGetError();
    if (val != GL_NO_ERROR) {
        QString msg = QString(cmd) + QString(" => ") + QString::number(val);
        msg += "[ ";
        msg += translateGLError(val).c_str();
        msg += " ]";
        msg += QString("::") + QString(fileName);
        msg += "::" + QString::number(line);
        qDebug() << msg;
        getchar();
        exit(0);
    }
}

static void clear_gl_buffers_GL33(struct geometry_base_t *geometry,
                                  OpenGLFunctions * GLptr)
{
    int c = sizeof(geometry->tmpvbos) / sizeof(geometry->tmpvbos[0]);
    if(geometry->is_binded){
        GL_CHK(glDeleteVertexArrays(1, &(geometry->vao)), GLptr);
        GL_CHK(glDeleteBuffers(c, geometry->tmpvbos), GLptr);
        if(geometry->has_quadindices){
            GL_CHK(glDeleteBuffers(1, &(geometry->quadibo)), GLptr);
        }

        if(geometry->has_triindices){
            GL_CHK(glDeleteBuffers(1, &(geometry->triibo)), GLptr);
        }

        if(geometry->instanced){
            GL_CHK(glDeleteBuffers(1, &(geometry->vboExtra)), GLptr);
        }

        geometry->is_binded = false;
    }
}

/**
 * For dynamic drawing (path) is better to use glBufferSubData
 * instead of re-creating the GPU buffers.
 */
void Graphics::geometry_simple_bind_GL33(struct geometry_simple_t *geometry,
                                         OpenGLFunctions *GLptr)
{
    if(geometry){
        if(geometry->data){
            if(geometry->is_binded){
                GL_CHK(glBindVertexArray(geometry->vao), GLptr);
                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo[0]), GLptr);
                GL_CHK(glBufferSubData(GL_ARRAY_BUFFER, 0,
                                       3 * sizeof(GLfloat) * geometry->data->size(),
                                       &(geometry->data->operator[](0))), GLptr);

                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo[1]), GLptr);
                GL_CHK(glBufferSubData(GL_ARRAY_BUFFER, 0,
                                       sizeof(glm::mat3) * geometry->extraEx->size(),
                                       &(geometry->extraEx->operator[](0))), GLptr);

               GL_CHK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (const GLvoid*)0), GLptr);
               GL_CHK(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (const GLvoid*)12), GLptr);
               GL_CHK(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (const GLvoid*)24), GLptr);
            }else{
                GL_CHK(glGenVertexArrays(1, &geometry->vao), GLptr);
                GL_CHK(glBindVertexArray(geometry->vao), GLptr);
                GL_CHK(glGenBuffers(2, geometry->vbo), GLptr);

                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo[0]), GLptr);
                GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                                    9 * sizeof(GLfloat) * MAX_TRIANGLES_PER_CALL,
                                    NULL, GL_DYNAMIC_DRAW), GLptr);

                GL_CHK(glBufferSubData(GL_ARRAY_BUFFER, 0,
                                       3 * sizeof(GLfloat) * geometry->data->size(),
                                       &(geometry->data->operator[](0))), GLptr);
                GL_CHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);

                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo[1]), GLptr);
                GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                                    3 * sizeof(glm::mat3) * MAX_TRIANGLES_PER_CALL,
                                    NULL, GL_DYNAMIC_DRAW), GLptr);

                GL_CHK(glBufferSubData(GL_ARRAY_BUFFER, 0,
                                       sizeof(glm::mat3) * geometry->extraEx->size(),
                                       &(geometry->extraEx->operator[](0))), GLptr);

                GL_CHK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (const GLvoid*)0), GLptr);
                GL_CHK(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (const GLvoid*)12), GLptr);
                GL_CHK(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (const GLvoid*)24), GLptr);

                geometry->is_binded = true;
            }
            GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, 0), GLptr);
            GL_CHK(glBindVertexArray(0), GLptr);
        }
    }
}

/**
 * In modern OpenGL we need to generate buffers to interact directly to the
 * GPU. This is done by the VertexArray, ArrayBuffer and ElementBuffer.
 * VertexArray represent binding points in the GPU, ArrayBuffer the actual
 * geometric data that needs to be rendered and ElementBuffer represents
 * a fast indexing table for the ArrayBuffer, this provides very fast
 * render times.
 *
 * Given a geometry_base_t @geometry, this functions handles the creation
 * and setup of the previous described buffers.
 */
void Graphics::geometry_bind_GL33(struct geometry_base_t *geometry,
                                  OpenGLFunctions * GLptr,
                                  bool instaceSupport)
{
    if(GLptr && geometry){ //assumes addresses are loaded
        int c = sizeof(geometry->tmpvbos) / sizeof(geometry->tmpvbos[0]);
        GLuint buffer = 0;
        clear_gl_buffers_GL33(geometry, GLptr);
        GL_CHK(glGenVertexArrays(1, &geometry->vao), GLptr);
        GL_CHK(glBindVertexArray(geometry->vao), GLptr);
        GL_CHK(glGenBuffers(c, geometry->tmpvbos), GLptr);
        int vbo_count = 2;
        if(geometry->nearPoints.size() > 0 && geometry->farPoints.size() > 0){
            vbo_count = 4;
        }

        if(geometry->has_triindices){
            GL_CHK(glGenBuffers(1, &geometry->triibo), GLptr);
            GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->triibo), GLptr);
            GL_CHK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                sizeof(GLushort) * geometry->triindices.size(),
                                &(geometry->triindices[0]), GL_STATIC_DRAW), GLptr);
            if(geometry->has_quadindices > 0){
                GL_CHK(glGenBuffers(1, &geometry->quadibo), GLptr);
                GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->quadibo), GLptr);
                GL_CHK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                    sizeof(GLushort) * geometry->quadindices.size(),
                                    &(geometry->quadindices[0]), GL_STATIC_DRAW), GLptr);
            }
        }

        if(geometry->positions.size() > 0){
            GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->tmpvbos[buffer]), GLptr);
            GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                                3 * sizeof(GLfloat) * geometry->positions.size(),
                                &(geometry->positions[0]), GL_STATIC_DRAW), GLptr);
            GL_CHK(glVertexAttribPointer(buffer++, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);
            if(geometry->normals.size() > 0){
                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->tmpvbos[buffer]), GLptr);
                GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                                    3 * sizeof(GLfloat) * geometry->normals.size(),
                                    &(geometry->normals[0]), GL_STATIC_DRAW), GLptr);
                GL_CHK(glVertexAttribPointer(buffer++, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);
            }

            if(vbo_count > 2){
                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->tmpvbos[buffer]), GLptr);
                GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                                    3 * sizeof(GLfloat) * geometry->nearPoints.size(),
                                    &(geometry->nearPoints[0]), GL_STATIC_DRAW), GLptr);
                GL_CHK(glVertexAttribPointer(buffer++, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);

                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->tmpvbos[3]), GLptr);
                GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                                    3 * sizeof(GLfloat) * geometry->farPoints.size(),
                                    &(geometry->farPoints[0]), GL_STATIC_DRAW), GLptr);
                GL_CHK(glVertexAttribPointer(buffer++, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);
            }

        }

        if(instaceSupport){
            if(vbo_count > 2){
                qDebug() << "INCORRECT BUFFER SETUP!";
            }else{
                int len = geometry->instancedModels.size();
                GL_CHK(glGenBuffers(1, &geometry->vboExtra), GLptr);
                GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->vboExtra), GLptr);
                GL_CHK(glBufferData(GL_ARRAY_BUFFER, len * sizeof(glm::mat4),
                                    &(geometry->instancedModels[0]), GL_STATIC_DRAW), GLptr);

                GLsizei vec4Size = sizeof(glm::vec4);
                GL_CHK(glEnableVertexAttribArray(2), GLptr);
                GL_CHK(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                                             4 * vec4Size, (void*)0), GLptr);
                GL_CHK(glEnableVertexAttribArray(3), GLptr);
                GL_CHK(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                                             4 * vec4Size, (void*)(vec4Size)), GLptr);
                GL_CHK(glEnableVertexAttribArray(4), GLptr);
                GL_CHK(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE,
                                             4 * vec4Size, (void*)(2 * vec4Size)), GLptr);
                GL_CHK(glEnableVertexAttribArray(5), GLptr);
                GL_CHK(glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE,
                                             4 * vec4Size, (void*)(3 * vec4Size)), GLptr);

                GL_CHK(glVertexAttribDivisor(2, 1), GLptr);
                GL_CHK(glVertexAttribDivisor(3, 1), GLptr);
                GL_CHK(glVertexAttribDivisor(4, 1), GLptr);
                GL_CHK(glVertexAttribDivisor(5, 1), GLptr);
            }
        }

        GL_CHK(glBindVertexArray(0), GLptr);
        GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, 0), GLptr);
        GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0), GLptr);
        geometry->instanced = instaceSupport;
        geometry->is_binded = true;
    }
}

static void target_generate_geometry_aligned(struct target_t *target){
    if(target){
        target->geometry->triangles = 0;
        target->geometry->quads = 0;
        float anchor = target->length;
        glm::vec3 normal(0,1,0);
        glm::vec3 a1(anchor/2.0f, target->baseHeight, 0.0f);
        glm::vec3 a2(-anchor/2.0f, target->baseHeight, anchor/2.0f);
        glm::vec3 a3(-anchor/4.0f, target->baseHeight, 0.0f);
        glm::vec3 a4(-anchor/2.0f, target->baseHeight, -anchor/2.0f);

        target->geometry->positions.push_back(a1);
        target->geometry->positions.push_back(a2);
        target->geometry->positions.push_back(a3);
        target->geometry->positions.push_back(a4);

        target->geometry->normals.push_back(normal);
        target->geometry->normals.push_back(normal);
        target->geometry->normals.push_back(normal);
        target->geometry->normals.push_back(normal);

        target->geometry->triindices.push_back(0);
        target->geometry->triindices.push_back(1);
        target->geometry->triindices.push_back(2);

        target->geometry->triindices.push_back(0);
        target->geometry->triindices.push_back(3);
        target->geometry->triindices.push_back(2);

        target->geometry->ptriangles.push_back(a1);
        target->geometry->ptriangles.push_back(a2);
        target->geometry->ptriangles.push_back(a3);

        target->geometry->ptriangles.push_back(a1);
        target->geometry->ptriangles.push_back(a4);
        target->geometry->ptriangles.push_back(a3);

        target->geometry->triangles = 2;
        target->geometry->has_triindices = true;
        target->geometry->has_quadindices = false;
    }
}

/*
 * Generates vertices, triangles and indices for a plane described by
 * @plane argument. This algorithm works as follows:
 *      1 - Generate a vector 'r' that is not aligned to the plane normal;
 *      2 - From the chosen vector generate perpendicular vector to both
 *          plane normal and 'r' vector, 's';
 *      3 - From the vectors 'r' and 's' fill the points by interpolating
 *          both vectors in the X, Y and Z directions starting from a base point
 *          in the plane, you now have all points in the plane that is perpendicular
 *          to the normal specified in @plane.
*/
static void plane_generate_geometry_aligned(struct plane_t *plane){
    if(plane){
        plane->geometry->triangles = 0;
        plane->geometry->quads = 0;
        bool chosen = false;
        glm::vec3 r;
        glm::vec3 uy(0,1,0); glm::vec3 ux(1,0,0);glm::vec3 uz(0,0,1);
        for(int i = 0; i < 3; i += 1){
            switch(i){
                case 0: r = uy; break;
                case 1: r = ux; break;
                case 2: r = uz; break;
            }
            glm::vec3 tr = glm::cross(r, plane->normal);
            float len = glm::length(tr);
            chosen = (len != 0.0f);
            if(chosen){
                break;
            }
        }

        if(chosen){
            glm::vec3 s = glm::cross(r, plane->normal);
            s = glm::normalize(s);
            float l = plane->sideL / static_cast<float>(plane->geometry->detail_level - 1);
            float l2 = plane->sideL / 2.0f;
            int detail_level = plane->geometry->detail_level / 2;
            int row_count = plane->geometry->detail_level;
            glm::vec3 v;
            int i = 0, j = 0;

            for(i = -detail_level; i <= detail_level; i += 1){
                for(j = -detail_level; j <= detail_level; j += 1){
                    v.x = plane->point.x + j * l * r.x + i * l * s.x;
                    v.y = plane->point.y + j * l * r.y + i * l * s.y;
                    v.z = plane->point.z + j * l * r.z + i * l * s.z;
                    plane->geometry->positions.push_back(v);
                    plane->geometry->normals.push_back(plane->normal);

                    /* Generates indices, this is for *_GL33 functions */
                    if(j < detail_level && i < detail_level){
                        int pi = i + detail_level;
                        int pj = j + detail_level;
                        int ni = pi * row_count + pj;
                        int ci = (pi + 1) * row_count + pj;
                        plane->geometry->triindices.push_back(ni + 1);
                        plane->geometry->triindices.push_back(ci);
                        plane->geometry->triindices.push_back(ni);

                        plane->geometry->triindices.push_back(ni + 1);
                        plane->geometry->triindices.push_back(ci);
                        plane->geometry->triindices.push_back(ci + 1);
                        plane->geometry->triangles += 2;
                    }
                }
            }

            plane->left  = plane->point + glm::vec3(-l2,  0,  l2);
            plane->right = plane->point + glm::vec3( l2 , 0, -l2);

            plane->geometry->has_triindices  = plane->geometry->triindices.size()  > 0;
            plane->geometry->has_quadindices = plane->geometry->quadindices.size() > 0;
        }
    }
}

static void bind_global_uniforms(QOpenGLShaderProgram *program, View *view_system,
                                 QVector4D baseColor, QVector4D lineColor,
                                 QMatrix4x4 modelMatrix, float segLen,
                                 int totalSegments)
{

    int matrixUniformLocation   = program->uniformLocation("projection");
    int viewUniformLocation     = program->uniformLocation("view");
    int modelUniformLocation    = program->uniformLocation("model");
    int viewportUniformLocation = program->uniformLocation("viewportMatrix");
    int colorUniformLocation    = program->uniformLocation("baseColor");
    int lineUniformLocation     = program->uniformLocation("lineColor");
    int segLenUniformLocation   = program->uniformLocation("segmentLength");
    int segCntUniformLocation   = program->uniformLocation("segmentCount");
    int zoomLvlUniformLocation  = program->uniformLocation("zoomLevel");
    int pDataUniformLocation    = program->uniformLocation("pData");

    Camera *camera = view_system->get_camera();
    if(matrixUniformLocation > -1)
        program->setUniformValue(matrixUniformLocation, view_system->get_projection_matrix());
    if(viewUniformLocation > -1)
        program->setUniformValue(viewUniformLocation, camera->get_view_matrix());
    if(viewUniformLocation > -1)
        program->setUniformValue(viewportUniformLocation, view_system->get_viewport_matrix());
    if(modelUniformLocation > - 1)
        program->setUniformValue(modelUniformLocation, modelMatrix);
    if(colorUniformLocation > - 1)
        program->setUniformValue(colorUniformLocation, baseColor);
    if(lineUniformLocation > - 1)
        program->setUniformValue(lineUniformLocation, lineColor);
    if(segLenUniformLocation > -1)
        program->setUniformValue(segLenUniformLocation, segLen);
    if(segCntUniformLocation > -1)
        program->setUniformValue(segCntUniformLocation, totalSegments);
    if(zoomLvlUniformLocation > -1)
        program->setUniformValue(zoomLvlUniformLocation, camera->currentZoomLevel+1);
    if(pDataUniformLocation > -1)
        program->setUniformValueArray(pDataUniformLocation,
                                      configSegments, MAX_SEGMENTS, 1);
}

template<typename Vec>
static void inner_render_triangulation_GL33(std::vector<Vec> *data, GLuint vao,
                                            QMatrix4x4 modelMatrix, QOpenGLShaderProgram *program,
                                            View *view_system, OpenGLFunctions *GLptr,
                                            QVector4D baseColor, QVector4D lineColor,
                                            float elevation)
{
    glm::vec2 segAndLen = get_segment_and_length();
    int len = SCAST(int, data->size());
    if(len > 0){
        int elevationLocation = -1;
        if(!program->isLinked()){
            program->link();
        }

        program->bind();
        GL_CHK(glBindVertexArray(vao), GLptr);
        GL_CHK(glEnableVertexAttribArray(0), GLptr);
        GL_CHK(glEnableVertexAttribArray(1), GLptr);
        GL_CHK(glEnableVertexAttribArray(2), GLptr);
        GL_CHK(glEnableVertexAttribArray(3), GLptr);

        bind_global_uniforms(program, view_system, baseColor, lineColor,
                             modelMatrix, segAndLen.y, SCAST(int, segAndLen.x));
        elevationLocation = program->uniformLocation("elevation");
        if(elevationLocation > -1)
            program->setUniformValue(elevationLocation, elevation);

        GL_CHK(glDrawArrays(GL_TRIANGLES, 0, len), GLptr);

        GL_CHK(glDisableVertexAttribArray(0), GLptr);
        GL_CHK(glDisableVertexAttribArray(1), GLptr);
        GL_CHK(glDisableVertexAttribArray(2), GLptr);
        GL_CHK(glDisableVertexAttribArray(3), GLptr);
        GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0), GLptr);
        GL_CHK(glBindVertexArray(0), GLptr);
        program->release();
    }
}

void Graphics::render_triangulation_GL33(struct geometry_simple_t *geometry, QOpenGLShaderProgram *program,
                                         View *view_system, OpenGLFunctions *GLptr,
                                         QVector4D baseColor, QVector4D lineColor)
{
    QMatrix4x4 model; model.setToIdentity();
    inner_render_triangulation_GL33<glm::vec3>(geometry->data, geometry->vao, model, program,
                                               view_system, GLptr, baseColor, lineColor,
                                               0.1f);
}

void Graphics::render_triangulation_GL33(struct geometry_base_t *geometry, QOpenGLShaderProgram *program,
                                         View *view_system, OpenGLFunctions *GLptr,
                                         QVector4D baseColor, QVector4D lineColor)
{
    inner_render_triangulation_GL33<glm::vec3>(&geometry->positions, geometry->vao,
                                               geometry->modelMatrix, program,
                                               view_system, GLptr, baseColor, lineColor,
                                               0.11f);
}

void Graphics::render_indexed_triangulation_GL33(struct geometry_base_t *geometry, QOpenGLShaderProgram *program,
                                                 View *view_system, OpenGLFunctions * GLptr,
                                                 QVector4D baseColor, QVector4D lineColor)
{
    glm::vec2 segAndLen = get_segment_and_length();
    static int warnMessage = 0;
    if(!program->isLinked()){
        program->link();
    }
    program->bind();
    GL_CHK(glBindVertexArray(geometry->vao), GLptr);
    GL_CHK(glEnableVertexAttribArray(0), GLptr);
    GL_CHK(glEnableVertexAttribArray(1), GLptr);
    GL_CHK(glEnableVertexAttribArray(2), GLptr);
    GL_CHK(glEnableVertexAttribArray(3), GLptr);

    bind_global_uniforms(program, view_system, baseColor, lineColor,
                         geometry->modelMatrix, segAndLen.y, SCAST(int, segAndLen.x));

    GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->triibo), GLptr);

    /* Temporary: Not ideal but I'm having trouble instancing geometry shaders, will take a look later */
    int count = static_cast<int>(geometry->qInstancedModels.size());

    if(count > 0){
        if(warnMessage < 1){
            qDebug() << "Warning: Inneficient rendering method call [" << count << "], "
                     << "you should use instancing rendering for this @geometry";
            warnMessage = 1;
        }

        int modelUniformLocation = program->uniformLocation("model");
//        int colorUniformLocation = program->uniformLocation("baseColor");
        for(int i = 0; i < count; i += 1){
            if(modelUniformLocation > -1){
                program->setUniformValue(modelUniformLocation, geometry->qInstancedModels.at(i));
            }

//            if(i == count/2 && colorUniformLocation > -1){
//                program->setUniformValue(colorUniformLocation, QVector4D(1.0, 0.0, 0.0, 1.0));
//            }else{
//                program->setUniformValue(colorUniformLocation, baseColor);
//            }

            GL_CHK(glDrawElements(GL_TRIANGLES, geometry->triindices.size(),
                                  GL_UNSIGNED_SHORT, (void *)0), GLptr);
        }
    }else{
        GL_CHK(glDrawElements(GL_TRIANGLES, geometry->triindices.size(),
                              GL_UNSIGNED_SHORT, (void *)0), GLptr);
    }

    GL_CHK(glDisableVertexAttribArray(0), GLptr);
    GL_CHK(glDisableVertexAttribArray(1), GLptr);
    GL_CHK(glDisableVertexAttribArray(2), GLptr);
    GL_CHK(glDisableVertexAttribArray(3), GLptr);
    GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0), GLptr);
    GL_CHK(glBindVertexArray(0), GLptr);
    program->release();
}

void Graphics::plane_grid_render_GL33(struct plane_grid_t *pGrid, QOpenGLShaderProgram *program,
                                       View *view_system, OpenGLFunctions * GLptr, GPSOptions options)
{
    struct geometry_base_t * geometry = pGrid->plane->geometry;
    geometry->modelMatrix.setToIdentity();
    QVector4D baseColor(options.floorSquareColor, 1.0);
    QVector4D lineColor(options.floorLinesColor, 1.0);
    render_indexed_triangulation_GL33(geometry, program, view_system, GLptr,
                                      baseColor, lineColor);
}

void Graphics::target_render_GL33(struct target_t *target, QOpenGLShaderProgram *program,
                                  View *view_system, OpenGLFunctions * GLptr, GPSOptions options)
{
    struct geometry_base_t * geometry = target->geometry;
    geometry->modelMatrix = Metrics::get_target_model_matrix(target);
    QVector4D baseColor(options.arrowColor, 1.0);
    render_indexed_triangulation_GL33(geometry, program, view_system, GLptr,
                                      baseColor, baseColor);
}

void Graphics::path_render_GL33(struct geometry_simple_t *geometry, QOpenGLShaderProgram *program,
                                View *view_system, OpenGLFunctions *GLptr, GPSOptions options)
{
    QMatrix4x4 model; model.setToIdentity();
    QVector4D baseColor(options.normalPathColor, 1.0);
    inner_render_triangulation_GL33<glm::vec3>(geometry->data, geometry->vao, model, program,
                                               view_system, GLptr, baseColor, baseColor,
                                               0.1f);
}

struct target_t * Graphics::target_new(float length, float baseHeight){
    struct target_t * target = new struct target_t;
    target->geometry = new struct geometry_base_t;
    target->length = length;
    target->baseHeight = baseHeight;
    target->geometry->detail_level = 1;
    target_generate_geometry_aligned(target);
    target->globalLocation = QVector3D(0,0,0);
    target->graphicalLocation = QVector3D(0, baseHeight, 0);
    target->lastGraphicalLocation = target->graphicalLocation;
    target->currentDirection = QVector3D(1,0,0);
    target->lastDirection = QVector3D(1,0,0);
    target->differentialAngle = 0.0;
    target->differentialDistance = 0.0;
    target->quaternionInited = 0.0;
    target->graphicalRotation = -1.0;
    target->geometry->is_binded = false;
    return target;
}

struct plane_t * Graphics::plane_new(glm::vec3 bpoint, glm::vec3 bnormal,
                                     float sidelen, int detail_level)
{
    glm::mat4 model(1.0f);
    struct plane_t *plane = new struct plane_t;
    plane->geometry = new struct geometry_base_t;
    plane->point = bpoint;
    plane->normal = bnormal;
    int levels = detail_level;
    if(levels % 2 == 0){ //we do not support odd levels
        levels += 1;
    }
    plane->geometry->detail_level = levels;
    plane->sideL = sidelen;
    plane_generate_geometry_aligned(plane);
    plane->geometry->instancedModels.push_back(model);
    plane->geometry->is_binded = false;
    return plane;
}

//struct geometry_base_t * Graphics::new_empty_geometry(){
//    struct geometry_base_t *geometry = new struct geometry_base_t;
//    geometry->triangles = 0;
//    geometry->quads = 0;
//    geometry->has_triindices = false;
//    geometry->has_quadindices = false;
//    geometry->is_binded  = false;
//    geometry->instanced = false;
//    return geometry;
//}

struct geometry_simple_t * Graphics::new_empty_simple_geometry(){
    struct geometry_simple_t *simple = new struct geometry_simple_t;
    simple->is_binded = false;
    return simple;
}

/****************************************************************************/
/*                 M E T R I C S      F U N C T I O N S                     */
/****************************************************************************/

static Orientation_t Orientation;
static QMutex orientationMutex, optionsMutex;
static Pathing path(0.0f);
static float currentElevation = 0.1f;
static GPSOptions gpsOptions;
static GPSOptions preGpsOptions;
static bool inLoad = false;
static glm::vec3 loadLastPos(0.0f), loadCurrPos(0.0f);
static int anyLoad = 0;

Voxel2D::VoxelWorld *voxWorld = nullptr;
GLfloat configSegments[MAX_SEGMENTS];
std::vector<glm::vec4> controlPoints;

static glm::vec2 get_segment_and_length(){
    QMutexLocker locker(&optionsMutex);
    return glm::vec2(gpsOptions.segments, gpsOptions.length);
}

static glm::vec3 get_segment_length_and_sample(){
    QMutexLocker locker(&optionsMutex);
    return glm::vec3(gpsOptions.segments, gpsOptions.length, gpsOptions.sampleCount);
}

GPSOptions Metrics::get_gps_option(){
    QMutexLocker locker(&optionsMutex);
    return gpsOptions;
}

void Metrics::set_line_state(uchar *linesMask)
{
    path.set_segment_state(linesMask);
}

void Metrics::set_line_state(int line, int isOn)
{
    path.set_segment_state(line, isOn);
}

// your risk
void Metrics::update_gps_options(GPSOptions options){
    QMutexLocker locker(&optionsMutex);
    gpsOptions = options;
}

void Metrics::initialize(GPSOptions options){
    gpsOptions = options;
    preGpsOptions = options;
    Vec2 dir{1.0f, 0.0f};
    Vec2 start{0.0f, 0.0f};
    Vec2 normal = Vec2Maths::normal(dir);
    Vec2 nNormal = Vec2Maths::multiply(normal, -options.length/2.0f);
    Vec2 pNormal = Vec2Maths::multiply(normal, options.length/2.0f);
    Vec2 segStt = Vec2Maths::add(pNormal, start);
    Vec2 segEnd = Vec2Maths::add(nNormal, start);

    Vec2 n = normal;

    if(!voxWorld){
        voxWorld = new Voxel2D::VoxelWorld(40.0f);
    }

    unsigned int totalPoints = SCAST(unsigned int, options.sampleCount * options.segments);
    totalPoints += 2;
    controlPoints.clear();
    int seg = 0;
    Vec2 A0 = segEnd;
    float xkAcc = 0.0f;
    controlPoints.push_back(glm::vec4(A0.x, 0.1f, A0.y, seg));
    for(seg = 0; seg < options.segments; seg += 1){
        Vec2 Ak = Vec2Maths::add(A0, Vec2Maths::multiply(n, xkAcc));
        float d = options.segmentsLength[seg] / SCAST(float, options.sampleCount + 1);
        float s = d;
        for(int i = 0; i < options.sampleCount; i += 1){
            Vec2 p = Vec2Maths::add(Ak, Vec2Maths::multiply(n, s));
            controlPoints.push_back(glm::vec4(p.x, 0.1f, p.y, seg));
            s += d;
        }

        xkAcc += options.segmentsLength[seg];
    }

    controlPoints.push_back(glm::vec4(segStt.x, 0.1f, segStt.y,
                                      options.segments-1));

    for(int i = 0; i < MAX_SEGMENTS; i += 1){
        if(i < options.segments){
            configSegments[i] = options.segmentsLength[i];
        }else{
            configSegments[i] = 0;
        }
    }

    path.set_segment_count(options.segments);
}

void Metrics::initialize()
{
    lock_for_series_update();
    int total = path.totalTriangles;
    Orientation_t empty;
    Orientation = empty;
    gpsOptions = preGpsOptions;
    Vec2 dir{1.0f, 0.0f};
    Vec2 start{0.0f, 0.0f};
    Vec2 normal = Vec2Maths::normal(dir);
    Vec2 nNormal = Vec2Maths::multiply(normal, -preGpsOptions.length/2.0f);
    Vec2 pNormal = Vec2Maths::multiply(normal, preGpsOptions.length/2.0f);
    Vec2 segStt = Vec2Maths::add(pNormal, start);
    Vec2 segEnd = Vec2Maths::add(nNormal, start);

    Vec2 n = normal;

    unsigned int totalPoints = SCAST(unsigned int, preGpsOptions.sampleCount * preGpsOptions.segments);
    totalPoints += 2;
    controlPoints.clear();
    int seg = 0;
    Vec2 A0 = segEnd;
    float xkAcc = 0.0f;
    controlPoints.push_back(glm::vec4(A0.x, 0.1f, A0.y, seg));
    for(seg = 0; seg < preGpsOptions.segments; seg += 1){
        Vec2 Ak = Vec2Maths::add(A0, Vec2Maths::multiply(n, xkAcc));
        float d = preGpsOptions.segmentsLength[seg] / SCAST(float, preGpsOptions.sampleCount + 1);
        float s = d;
        for(int i = 0; i < preGpsOptions.sampleCount; i += 1){
            Vec2 p = Vec2Maths::add(Ak, Vec2Maths::multiply(n, s));
            controlPoints.push_back(glm::vec4(p.x, 0.1f, p.y, seg));
            s += d;
        }

        xkAcc += preGpsOptions.segmentsLength[seg];
    }

    controlPoints.push_back(glm::vec4(segStt.x, 0.1f, segStt.y,
                                      preGpsOptions.segments-1));

    for(int i = 0; i < MAX_SEGMENTS; i += 1){
        if(i < preGpsOptions.segments){
            configSegments[i] = preGpsOptions.segmentsLength[i];
        }else{
            configSegments[i] = 0;
        }
    }
    path.reset_state();
    path.set_segment_count(preGpsOptions.segments);
//    path.totalTriangles = total;
    unlock_series_update();
}

void Metrics::finalize(GPSOptions options)
{
    options.reset_sessions();

    controlPoints.clear();
    memset(configSegments, 0, sizeof(configSegments));
    path.set_segment_count(0);

    delete voxWorld;
    voxWorld = nullptr;
}

float Metrics::compute_xz_distance(QVector3D eye, QVector3D target){
    QVector3D d = eye - target;
    float x2 = d.x() * d.x();
    float z2 = d.z() * d.z();
    float xz = qSqrt(z2 + x2);
    return xz;
}

//float Metrics::get_graphical_rotation(qreal rotation){
//    return static_cast<float>(360.0 - rotation);
//}

QVector3D Metrics::get_camera_object_vector(QVector3D graphicalLocation,
                                            QVector3D currentDirection,
                                            QVector3D source,
                                            float base_distance)
{
    QVector3D source2(source.x(), 0.0, source.z());
    QVector3D targ2(graphicalLocation.x(),
                    0.0, graphicalLocation.z());

    QVector3D dir = -currentDirection;
    QVector3D dir2(dir.x(), 0.0, dir.z());
    dir2.normalize();
    QVector3D resp = targ2 + dir2 * base_distance;
    resp.setY(source.y());
    return resp;
}

static glm::quat make_quaternion(QVector3D &dir, QVector3D &old){
    old.normalize();
    dir.normalize();
    glm::vec3 fromVec(old.x(), old.y(), old.z());
    glm::vec3 toVec(dir.x(), dir.y(), dir.z());
    return glm::quat(toVec, fromVec);
}

QMatrix4x4 Metrics::get_target_model_matrix(struct target_t *target){
    QMatrix4x4 model;
    QVector3D dir = target->currentDirection;
    QVector3D old = target->lastDirection;

    if(target->graphicalLocation != target->lastGraphicalLocation){
        target->currentDirection = target->graphicalLocation - target->lastGraphicalLocation;
        target->currentDirection.normalize();
    }

    target->rotationQuaternion = make_quaternion(dir, old);

    glm::mat4 rot = glm::toMat4(target->rotationQuaternion);
    QMatrix4x4 R(glm::value_ptr(rot));
    model.setToIdentity();
    model.translate(target->graphicalLocation);
    model = model * R;
    return model;
}

//void Metrics::set_initial_orientation(Orientation_t orientation){
//    QMutexLocker locker(&orientationMutex);
//    Orientation = orientation;
//    Orientation.changed = 1;
//}

Orientation_t Metrics::get_orientation_unsafe(/*std::vector<glm::vec4> *vec*/){
    Orientation_t resp = Orientation;
    resp.minElevation = currentElevation+Metrics::min_dY;
//    if(vec){
//        vec->clear();
//        int size = SCAST(int, controlPoints.size());
//        for(int i = 0; i < size; i += 1){
//            vec->push_back(controlPoints.at(i));
//        }
//    }
    return resp;
}

Orientation_t Metrics::get_orientation_safe(std::vector<glm::vec4> *vec, int clean){
     Orientation_t resp;
    if(!inLoad){
        QMutexLocker locker(&orientationMutex);
        resp = Orientation;
        if(clean != 0) Orientation.changed = 0;
        resp.minElevation = currentElevation+Metrics::min_dY;
        if(vec){
            vec->clear();
            int size = SCAST(int, controlPoints.size());
            for(int i = 0; i < size; i += 1){
                vec->push_back(controlPoints.at(i));
            }
        }
    }else{
        resp.changed = 1;
    }

    return resp;
}

//bool Metrics::orientation_changed(){
//    return Orientation.changed;
//}

void Metrics::lock_for_series_update(){
    orientationMutex.lock();
}

void Metrics::unlock_series_update(){
    orientationMutex.unlock();
}

double Metrics::azimuth_to_sphere(double azimuth){
    double angle = 0.0;
    if(azimuth >= 0 && azimuth <= 90){
        angle = 90 - azimuth;
    }else if(azimuth > 90 && azimuth <= 360){
        angle = 270 + (180 - azimuth);
    }
    return angle;
}

//unsigned char* Metrics::get_current_move_mask_unsafe_ex(){
//    return path.movementHitMaskEx;
//}

void Metrics::add_triangleEx(Vec2 v0, Vec2 v1, Vec2 v2, unsigned char* mask)
{
    voxWorld->quadtree_insert_triangleEx(v0, v1, v2, path.currentHitMaskEx,
                                         mask, path.totalTriangles,
                                         currentElevation+0.01f);
}

void Metrics::load_start(){
    orientationMutex.lock();
    inLoad = true;
    anyLoad = 0;
}

void Metrics::load_updateEx(QGeoCoordinate newLocation, unsigned char* moveMask){
    memcpy(path.movementHitMaskEx,moveMask,MAX_MASK_SEG);
    int changed = 0;
    Q_UNUSED(Metrics::update_orientation(newLocation, changed));
    loadLastPos = loadCurrPos;
    loadCurrPos = Orientation.pos;
    anyLoad = 1;
}

int Metrics::get_load_vectors(glm::vec3 &last, glm::vec3 &curr,
                              bool &isLoading, float &elevation)
{
    isLoading = inLoad;
    if(!inLoad){
        last = loadLastPos;
        curr = loadCurrPos;
        elevation = currentElevation + Metrics::min_dY;
        return anyLoad;
    }
    return -1;
}

void Metrics::load_finish(){
    inLoad = false;
    orientationMutex.unlock();
}

/**
 * Given a new GPS position compute the new position of the object
 * and update the path, voxel and control points.
 */
update_data Metrics::update_orientation(QGeoCoordinate newLocation, int &changed){
    changed = 0;

    /* Test if it is the first GPS location */
    glm::vec3 segAndLenAndCount = get_segment_length_and_sample();
    if(Orientation.valid == 1){
        /* It is not the first, compute azimuth and distance in meters */
        glm::vec3 north(0.0f, 0.0f, 1.0f); //our north is +Z
        glm::vec3 prev = Orientation.dir;
        double distance  = Orientation.geoCoord.distanceTo(newLocation);
        double azimuthTo = Orientation.geoCoord.azimuthTo(newLocation);

        /* Compute rotation angle so we can obtain where the virtual north is now */
        double rAngle    = glm::radians(azimuthTo);//azimuthTo is degrees
        double sinAngle  = glm::sin(rAngle);
        double cosAngle  = glm::cos(rAngle);

        /* Apply rotation to the north vector */
        double dX = north.x * cosAngle  + north.z * sinAngle;
        double dY = north.y;                                 //we don't handle elevation
        double dZ = -north.x * sinAngle + north.z * cosAngle;

        /* Compute transition and orientation */
        glm::vec3 newDir(dX, dY, dZ);
        newDir = glm::normalize(newDir);//I'm sure this is already unit but just in case...

        double dA = SCAST(double, glm_extend::dp_angle(newDir, prev));
        glm::vec3 transitionReal(dX * distance, dY * distance, dZ * distance);
        Orientation.realDir = newDir;
        Orientation.realPos = Orientation.realPos + transitionReal;

        glm::vec3 transitionGraphical(dX * distance, dY * distance, dZ * distance);
        glm::vec3 graphicalPoint = Orientation.pos + transitionGraphical;
        glm::vec3 target = graphicalPoint;
        bool insert = true;

        if(gpsOptions.filterMovement){
            insert = (distance > Metrics::min_dD);
        }

        if(insert){
            Vec2 pathPoint{target.x, target.z};
            Vec2 dir2{newDir.x, newDir.z};

            Vec2 normal  = Vec2Maths::normal(dir2);
            Vec2 nNormal = Vec2Maths::multiply(normal, -segAndLenAndCount.y/2.0f);
            Vec2 pNormal = Vec2Maths::multiply(normal, segAndLenAndCount.y/2.0f);
            Vec2 segStt  = Vec2Maths::add(pNormal, pathPoint);
            Vec2 segEnd  = Vec2Maths::add(nNormal, pathPoint);

            Vec2 n = normal;
            int seg = 0;
            unsigned int oldHitState = 0x00;
            memset(path.currentHitMaskEx,0,MAX_MASK_SEG);//rev

            /* Assure the center voxel is correct */
            voxWorld->update_center_voxel(Vec2{target.x, target.z});
            if(voxWorld->intersectsAnything(segEnd, path.totalTriangles,
                                            oldHitState, currentElevation,
                                            segAndLenAndCount.x))
            {
                int stateEx = BitHelper::single_bit_is_setEx(path.movementHitMaskEx, 0);

                if (oldHitState && stateEx)
                    BitHelper::single_bit_setEx(path.currentHitMaskEx, seg);
            }

            unsigned int amount = 0;
            Vec2 A0 = segEnd;
            float xkAcc = 0.0f;
            controlPoints[amount++] = (glm::vec4(segEnd.x, 0.1f, segEnd.y, 0));
            for(seg = 0; seg < segAndLenAndCount.x; seg += 1){
                Vec2 Ak = Vec2Maths::add(A0, Vec2Maths::multiply(n, xkAcc));
                float d = configSegments[seg] / SCAST(float, segAndLenAndCount.z + 1);
                float s = d;
                for(int i = 0; i < segAndLenAndCount.z; i += 1){
                    Vec2 p = Vec2Maths::add(Ak, Vec2Maths::multiply(n, s));
                    s += d;
                    controlPoints[amount++] = (glm::vec4(p.x, 0.1f, p.y, seg));
                    if(voxWorld->intersectsAnything(p, path.totalTriangles,
                                                    oldHitState, currentElevation,
                                                    segAndLenAndCount.x))
                    {
                        int tmp = seg > segAndLenAndCount.x-1 ? segAndLenAndCount.x-1 : seg;
                        int stateEx = BitHelper::single_bit_is_setEx(path.movementHitMaskEx, tmp);

                        if (oldHitState && stateEx)
                            BitHelper::single_bit_setEx(path.currentHitMaskEx, seg);
                    }
                }
                xkAcc += configSegments[seg];
            }

            if(voxWorld->intersectsAnything(segStt, path.totalTriangles,
                                            oldHitState, currentElevation,
                                            segAndLenAndCount.x))
            {
                int stateEx = BitHelper::single_bit_is_setEx(path.movementHitMaskEx, segAndLenAndCount.x-1);

                if(oldHitState && stateEx)
                    BitHelper::single_bit_setEx(path.currentHitMaskEx, segAndLenAndCount.x-1);
            }

            controlPoints[amount++] = (glm::vec4(segStt.x, 0.1f, segStt.y,
                                                 segAndLenAndCount.x-1));

            /* Insert triangle in path, this will trigger voxel insertion */
            Polyline2D::insertOne(&path, pathPoint, segAndLenAndCount.y);

            /* Apply new coordinates */
            Orientation.prevDir = Orientation.dir;
            Orientation.fromNorthAngle = azimuthTo;
            Orientation.dir = newDir;
            Orientation.distance = SCAST(float, distance);
            Orientation.pos = target;
            Orientation.geoCoord = newLocation;
            Orientation.changed = 1;
            Orientation.dX = dX;
            Orientation.dY = dY;
            Orientation.dZ = dZ;
            Orientation.dA = dA;
            Orientation.is_first = 0;

            changed = 1;
        }
    }else{
        /* First position, mark this position as the starting point */
        Orientation.valid = 1;
        Orientation.fromNorthAngle = -1.0;
        Orientation.geoCoord = newLocation;
        Orientation.changed = 1;
        changed = 1;
    }

    update_data ret;
    ret.x = Orientation.pos.x;
    ret.z = Orientation.pos.z;
    ret.currentHitMaskEx = path.currentHitMaskEx;
    ret.movementMaskEx = path.movementHitMaskEx;
    ret.segments = segAndLenAndCount.x;
    return ret;
}

void Metrics::setPath(Pathing *prePath){
    memcpy(prePath->currentHitMaskEx, path.currentHitMaskEx, MAX_MASK_SEG);
    memcpy(prePath->movementHitMaskEx, path.movementHitMaskEx, MAX_MASK_SEG);
    prePath->lineCount = path.lineCount;
    prePath->thickness = path.thickness;
    prePath->totalTriangles = path.totalTriangles;
    prePath->dynamicGeometry = path.dynamicGeometry;
    prePath->segments = path.segments;
    prePath->dynSeg = path.dynSeg;
    prePath->lastSegment = path.lastSegment;
    prePath->lastPoint = path.lastPoint;
    prePath->lastPolyPoint = path.lastPolyPoint;
    prePath->nextStart1 = path.nextStart1;
    prePath->nextStart2 = path.nextStart2;
    prePath->start1 = path.start1;
    prePath->start2 = path.start2;
    prePath->end1 = path.end1;
    prePath->end2 = path.end2;
}

void Metrics::addPath(Pathing prePath){
    memcpy(path.currentHitMaskEx, prePath.currentHitMaskEx, MAX_MASK_SEG);
    memcpy(path.movementHitMaskEx, prePath.movementHitMaskEx, MAX_MASK_SEG);
    path.lineCount = prePath.lineCount;
    path.thickness = prePath.thickness;
    path.totalTriangles = prePath.totalTriangles;
    path.dynamicGeometry = prePath.dynamicGeometry;
    path.segments = prePath.segments;
    path.dynSeg = prePath.dynSeg;
    path.lastSegment = prePath.lastSegment;
    path.lastPoint = prePath.lastPoint;
    path.lastPolyPoint = prePath.lastPolyPoint;
    path.nextStart1 = prePath.nextStart1;
    path.nextStart2 = prePath.nextStart2;
    path.start1 = prePath.start1;
    path.start2 = prePath.start2;
    path.end1 = prePath.end1;
    path.end2 = prePath.end2;
}

/****************************************************************************/
/*                 D E B U G G E R      F U N C T I O N S                   */
/****************************************************************************/

QOpenGLShaderProgram * GraphicsDebugger::debuggerShader = nullptr;
static struct geometry_base_t *debuggerGeometry = nullptr;
static struct geometry_base_t *debuggerVoxelGeometry = nullptr;

void GraphicsDebugger::clear_simple_geometry_GL33(struct geometry_base_t *geometry,
                                                  OpenGLFunctions *GLptr)
{
    if(geometry->is_binded){
        GL_CHK(glDeleteVertexArrays(1, &(geometry->vao)), GLptr);
        GL_CHK(glDeleteBuffers(2, geometry->tmpvbos), GLptr);
        geometry->is_binded = false;
    }
}

void GraphicsDebugger::bind_simple_geometry_GL33(struct geometry_base_t *geometry,
                                                OpenGLFunctions *GLptr)
{
    if(geometry){
        GL_CHK(glGenVertexArrays(1, &geometry->vao), GLptr);
        GL_CHK(glBindVertexArray(geometry->vao), GLptr);
        GL_CHK(glGenBuffers(2, geometry->tmpvbos), GLptr);

        GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->tmpvbos[0]), GLptr);
        GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                            3 * sizeof(GLfloat) * geometry->positions.size(),
                            &(geometry->positions[0]), GL_STATIC_DRAW), GLptr);
        GL_CHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);

        GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, geometry->tmpvbos[1]), GLptr);
        GL_CHK(glBufferData(GL_ARRAY_BUFFER,
                            3 * sizeof(GLfloat) * geometry->normals.size(),
                            &(geometry->normals[0]), GL_STATIC_DRAW), GLptr);
        GL_CHK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL), GLptr);

        GL_CHK(glBindBuffer(GL_ARRAY_BUFFER, 0), GLptr);
        GL_CHK(glBindVertexArray(0), GLptr);

        geometry->instanced = false;
        geometry->is_binded = true;
    }
}

void GraphicsDebugger::push_geometry(std::vector<glm::vec4> points,
                                     OpenGLFunctions *GLptr)
{
    if(!debuggerGeometry){
        debuggerGeometry = new struct geometry_base_t;
        debuggerGeometry->is_binded = false;
    }

    GraphicsDebugger::clear_simple_geometry_GL33(debuggerGeometry, GLptr);

    debuggerGeometry->positions.clear();
    debuggerGeometry->normals.clear();

    int len = SCAST(int, points.size());
    for(int i = 0; i < len; i += 1){
        glm::vec4 sp4(points.at(i));
        glm::vec3 sp(sp4.x, sp4.y, sp4.z);
        sp.y += 1.1f;
        debuggerGeometry->normals.push_back(glm::vec3(0,1,0));
        debuggerGeometry->positions.push_back(sp);
    }
}

//void GraphicsDebugger::render_decomposed_GL33(std::vector<glm::vec3>points, View *view_system,
//                                              QVector3D start, QVector3D end, int hash,
//                                              OpenGLFunctions * GLptr)
//{
//    int len = SCAST(int, points.size());
//    int count = len / hash;
//    float idx = (end.x() - start.x()) / SCAST(float, count);
//    float idy = (end.y() - start.y()) / SCAST(float, count);
//    float idz = (end.z() - start.z()) / SCAST(float, count);

//    int it = 0;
//    for(int i = 0; i < len; i += hash){
//        std::vector<glm::vec4> ppoints;
//        for(int j = 0; j < hash; j += 1){
//            glm::vec3 tpoint = points.at(i + j);
//            ppoints.push_back(glm::vec4(tpoint, 1.0));
//        }

//        float r = start.x() + it * idx;
//        float g = start.y() + it * idy;
//        float b = start.z() + it * idz;

//        QVector3D col(r, g, b);
//        GraphicsDebugger::render_points_GL33(ppoints, view_system,
//                                             col, GLptr);

//        it += 1;
//    }
//}

//void GraphicsDebugger::render_triangles_from_GL33(struct geometry_base_t *geometry, View *view_system,
//                                                  QVector3D startCol, QVector3D endCol, int startIdx,
//                                                  int endIdx, OpenGLFunctions *GLptr)
//{
//    if(GraphicsDebugger::debuggerShader){
//        glm::vec2 segAndLen = get_segment_and_length();
//        int len = endIdx - startIdx + 1;
//        int count = len * 3;
//        float idx = (endCol.x() - startCol.x()) / SCAST(float, len);
//        float idy = (endCol.y() - startCol.y()) / SCAST(float, len);
//        float idz = (endCol.z() - startCol.z()) / SCAST(float, len);

//        int it = 0;
//        debuggerShader->bind();
//        Graphics::geometry_bind_GL33(geometry, GLptr);
//        GL_CHK(glBindVertexArray(geometry->vao), GLptr);
//        GL_CHK(glEnableVertexAttribArray(0), GLptr);
//        GL_CHK(glEnableVertexAttribArray(1), GLptr);
//        GL_CHK(glEnableVertexAttribArray(2), GLptr);
//        GL_CHK(glEnableVertexAttribArray(3), GLptr);

//        QMatrix4x4 model; model.setToIdentity();

//        GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->triibo), GLptr);
//        int tid = startIdx * 3;
//        for(int i = 0; i < len; i += 1){
//            float r = startCol.x() + it * idx;
//            float g = startCol.y() + it * idy;
//            float b = startCol.z() + it * idz;

//            QVector4D color4(r, g, b, 1.0);
//            bind_global_uniforms(debuggerShader, view_system,
//                                 color4, color4, model, segAndLen.y,
//                                 SCAST(int, segAndLen.x));

//            GL_CHK(glDrawElements(GL_TRIANGLES, 3,
//                                  GL_UNSIGNED_SHORT,
//                                  (void *)(tid * sizeof(GLushort))), GLptr);
//            it += 1;
//            tid += 3;
//        }

//        GL_CHK(glDisableVertexAttribArray(0), GLptr);
//        GL_CHK(glDisableVertexAttribArray(1), GLptr);
//        GL_CHK(glDisableVertexAttribArray(2), GLptr);
//        GL_CHK(glDisableVertexAttribArray(3), GLptr);
//        GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0), GLptr);
//        GL_CHK(glBindVertexArray(0), GLptr);

//        debuggerShader->release();
//    }else{
//       qDebug() << "Debug shader not initialized!";
//    }
//}

void GraphicsDebugger::render_voxel_GL33(Voxel2D::Voxel *vox, View *view_system,
                                         QVector3D color, OpenGLFunctions *GLptr)
{
    if(GraphicsDebugger::debuggerShader){
        if(vox){
            if(!debuggerVoxelGeometry){
                static std::vector<glm::vec3> cube_data = {
                    {-0.5f,-0.5f,-0.5f},
                    {-0.5f,-0.5f, 0.5f},
                    {-0.5f, 0.5f, 0.5f},
                    {0.5f, 0.5f, -0.5f},
                    {-0.5f,-0.5f,-0.5f},
                    {-0.5f, 0.5f,-0.5f},
                    {0.5f,-0.5f,  0.5f},
                    {-0.5f,-0.5f,-0.5f},
                    {0.5f,-0.5f, -0.5f},
                    {0.5f, 0.5f, -0.5f},
                    {0.5f,-0.5f, -0.5f},
                    {-0.5f,-0.5f,-0.5f},
                    {-0.5f,-0.5f,-0.5f},
                    {-0.5f, 0.5f, 0.5f},
                    {-0.5f, 0.5f,-0.5f},
                    {0.5f,-0.5f,  0.5f},
                    {-0.5f,-0.5f, 0.5f},
                    {-0.5f,-0.5f,-0.5f},
                    {-0.5f, 0.5f, 0.5f},
                    {-0.5f,-0.5f, 0.5f},
                    {0.5f,-0.5f,  0.5f},
                    {0.5f, 0.5f,  0.5f},
                    {0.5f,-0.5f, -0.5f},
                    {0.5f, 0.5f, -0.5f},
                    {0.5f,-0.5f, -0.5f},
                    {0.5f, 0.5f,  0.5f},
                    {0.5f,-0.5f,  0.5f},
                    {0.5f, 0.5f,  0.5f},
                    {0.5f, 0.5f, -0.5f},
                    {-0.5f, 0.5f,-0.5f},
                    {0.5f, 0.5f,  0.5f},
                    {-0.5f, 0.5f,-0.5f},
                    {-0.5f, 0.5f, 0.5f},
                    {0.5f, 0.5f,  0.5f},
                    {-0.5f, 0.5f, 0.5f},
                    {0.5f,-0.5f,  0.5f}
                };
                debuggerVoxelGeometry = new geometry_base_t;
                debuggerVoxelGeometry->positions = cube_data;
                debuggerVoxelGeometry->normals   = cube_data;
            }

            glm::vec2 segAndLen = get_segment_and_length();

            debuggerShader->bind();
            GraphicsDebugger::bind_simple_geometry_GL33(debuggerVoxelGeometry, GLptr);
            GL_CHK(glBindVertexArray(debuggerVoxelGeometry->vao), GLptr);
            GL_CHK(glEnableVertexAttribArray(0), GLptr);
            GL_CHK(glEnableVertexAttribArray(1), GLptr);
            GL_CHK(glEnableVertexAttribArray(2), GLptr);
            GL_CHK(glEnableVertexAttribArray(3), GLptr);

            QMatrix4x4 model;
            float scale = vox->l2 * 2.00f * 0.99f;
            model.setToIdentity();

            model.translate(QVector3D(vox->center.x, 2.01f, vox->center.y));
            model.scale(QVector3D(scale, 2.0f, scale));
            QVector4D color4(color, 1.0);
            bind_global_uniforms(debuggerShader, view_system,
                                 color4, color4, model, segAndLen.y,
                                 SCAST(int, segAndLen.x));

//            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//            GL_CHK(glDrawArrays(GL_TRIANGLES, 0, 36), GLptr);
//            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            GL_CHK(glDisableVertexAttribArray(0), GLptr);
            GL_CHK(glDisableVertexAttribArray(1), GLptr);
            GL_CHK(glDisableVertexAttribArray(2), GLptr);
            GL_CHK(glDisableVertexAttribArray(3), GLptr);
            GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0), GLptr);
            GL_CHK(glBindVertexArray(0), GLptr);
            debuggerShader->release();
        }else{
//            qDebug() << "Invalid voxel";
        }
    }else{
        qDebug() << "Debug shader not initialized!";
    }
}

//void GraphicsDebugger::render_triangles_GL33(struct geometry_base_t *geometry, View *view_system,
//                                             QVector3D start, QVector3D end, OpenGLFunctions *GLptr)
//{
//    if(GraphicsDebugger::debuggerShader){

//        int len = SCAST(int, geometry->triindices.size());
//        int count = len / 3;
//        float idx = (end.x() - start.x()) / SCAST(float, count);
//        float idy = (end.y() - start.y()) / SCAST(float, count);
//        float idz = (end.z() - start.z()) / SCAST(float, count);

//        int it = 0;
//        debuggerShader->bind();
//        Graphics::geometry_bind_GL33(geometry, GLptr);
//        GL_CHK(glBindVertexArray(geometry->vao), GLptr);
//        GL_CHK(glEnableVertexAttribArray(0), GLptr);
//        GL_CHK(glEnableVertexAttribArray(1), GLptr);
//        GL_CHK(glEnableVertexAttribArray(2), GLptr);
//        GL_CHK(glEnableVertexAttribArray(3), GLptr);

//        QMatrix4x4 model; model.setToIdentity();
//        glm::vec2 segAndLen = get_segment_and_length();

//        GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->triibo), GLptr);
//        int tid = 0;
//        for(int i = 0; i < count; i += 1){
//            float r = start.x() + it * idx;
//            float g = start.y() + it * idy;
//            float b = start.z() + it * idz;

//            QVector4D color4(r, g, b, 1.0);
//            bind_global_uniforms(debuggerShader, view_system,
//                                 color4, color4, model, segAndLen.y,
//                                 SCAST(int, segAndLen.x));

//            GL_CHK(glDrawElements(GL_TRIANGLES, 3,
//                                  GL_UNSIGNED_SHORT,
//                                  (void *)(tid * sizeof(GLushort))), GLptr);

//            it += 1;
//            tid += 3;
//        }

//        GL_CHK(glDisableVertexAttribArray(0), GLptr);
//        GL_CHK(glDisableVertexAttribArray(1), GLptr);
//        GL_CHK(glDisableVertexAttribArray(2), GLptr);
//        GL_CHK(glDisableVertexAttribArray(3), GLptr);
//        GL_CHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0), GLptr);
//        GL_CHK(glBindVertexArray(0), GLptr);

//        debuggerShader->release();

//    }else{
//        qDebug() << "Debug shader not initialized!";
//    }
//}

void GraphicsDebugger::render_points_GL33(std::vector<glm::vec4>points,
                                          View *view_system, QVector3D color,
                                          OpenGLFunctions * GLptr)
{
    if(GraphicsDebugger::debuggerShader){
        glm::vec2 segAndLen = get_segment_and_length();
        GraphicsDebugger::push_geometry(points, GLptr);

        QMatrix4x4 model; model.setToIdentity();
        QVector4D color4(color, 1.0);

        GraphicsDebugger::bind_simple_geometry_GL33(debuggerGeometry, GLptr);
        debuggerShader->bind();
        GL_CHK(glBindVertexArray(debuggerGeometry->vao), GLptr);
        GL_CHK(glEnableVertexAttribArray(0), GLptr);
        GL_CHK(glEnableVertexAttribArray(1), GLptr);

        bind_global_uniforms(debuggerShader, view_system,
                             color4, color4, model, segAndLen.y,
                             SCAST(int, segAndLen.x));
        int len = SCAST(int, points.size());

        /*
         * Where is glPointSize in Android ?
        */
//        glPointSize(4.0f);
        GL_CHK(glDrawArrays(GL_POINTS, 0, len), GLptr);
//        glPointSize(1.0f);

        GL_CHK(glBindVertexArray(0), GLptr);
        GL_CHK(glDisableVertexAttribArray(0), GLptr);
        GL_CHK(glDisableVertexAttribArray(1), GLptr);
        debuggerShader->release();

    }else{
        qDebug() << "Debug shader not initialized!";
    }
}
