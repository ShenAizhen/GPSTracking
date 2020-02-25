#ifndef GPSOPTIONS_H
#define GPSOPTIONS_H
#include <QVector3D>
#include <QVector>
#include <common.h>
#include <database.h>

class GPSOptions
{
public:
    GPSOptions();
    GPSOptions(QVector<float> segmentsLen);
    GPSOptions(QVector<float> segmentsLen, int samples);
    GPSOptions(DBSession session);
    void make_default(bool reset_debug=true);
//    void make_colors_default();
    void apply_session(DBSession session);
    void reset_sessions();
    static void setValue(QVector3D *target, QVector3D value,
                         int normalize=0);

private:
    void internal_init(QVector<float> segmentsLen);
    void internal_init();
    void reset_debug_vars();
public:
    GLfloat segmentsLength[MAX_SEGMENTS];
    float length;
    int segments;

    QVector3D floorSquareColor;
    QVector3D floorLinesColor;
    QVector3D arrowColor;
    QVector3D normalPathColor;
    QVector3D fontColor;

    int sampleCount;
    bool filterMovement;

    // debug variables
    bool enableDebugVars;
    QVector3D debugCPointsColor;
    QVector3D debugVoxelPrimaryColor;
    QVector3D debugVoxelNeighboorColor;
    QVector3D debugVoxelContainerColor;
    QVector3D debugVoxelError;
    bool renderWireframe;
    bool renderControlPoints;
};

#endif // GPSOPTIONS_H
