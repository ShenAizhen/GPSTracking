#include "gpsoptions.h"
#include <QVector4D>

constexpr QVector3D floorColorDefault = QVector3D(0.96078f, 0.89411f, 0.8039215f);
constexpr QVector3D greenColorDefault = QVector3D(0.46078f, 0.89411f, 0.3039215f);
constexpr QVector3D redColorDefault   = QVector3D(0.93078f, 0.39411f, 0.3039215f);
constexpr QVector3D edgeColorDefault  = QVector3D(0.858823f, 0.815686f, 0.760784f);
//constexpr QVector3D edgeColor  = QVector3D(0.0f, 0.0f, 0.0f);
//constexpr QVector3D arrowColor = QVector3D(0.85f, 0.85f, 0.85f);
//constexpr QVector3D arrowColor = QVector3D(0.58039f, 0.0f,0.82745f);
constexpr QVector3D arrowColorDefault = QVector3D(0.28039f, 0.48039f,0.82745f);

GPSOptions::GPSOptions(QVector<float> segmentsLen){
    internal_init(segmentsLen);
    make_default();
}

GPSOptions::GPSOptions(DBSession session){
    apply_session(session);
    make_default();
}

GPSOptions::GPSOptions(){
    internal_init();
    make_default();
}

GPSOptions::GPSOptions(QVector<float> segmentsLen, int _samples){
    internal_init(segmentsLen);
    make_default();
    if(_samples > 0){
        sampleCount = _samples;
    }
}

void GPSOptions::reset_debug_vars(){
    renderControlPoints = false;
    filterMovement = true;
    enableDebugVars = false;
}

void GPSOptions::internal_init(){
    segments = -1;
    sampleCount = -1;
    renderWireframe = false;
    reset_debug_vars();
}

void GPSOptions::apply_session(DBSession session){
    QVector<float> segmentsLen;
    QStringList data = session.segments.split("|");
    for(QString &value : data){
        segmentsLen.push_back(value.toDouble());
    }
    internal_init(segmentsLen);
    if(session.samples > 0){
        sampleCount = session.samples;
    }
}

void GPSOptions::reset_sessions()
{
    internal_init(); // this will reset variables
}

void GPSOptions::internal_init(QVector<float> segmentsLen){
    internal_init();
    int size = segmentsLen.size();
    length = 0;
    if(size > MAX_SEGMENTS){
        qDebug() << "Warning: Too many segments given, max value is " << MAX_SEGMENTS
                 << " truncating this value";
        size = MAX_SEGMENTS;
    }

    for(int i = 0; i < size; i += 1){
        segmentsLength[i] = segmentsLen.at(i);
        length += segmentsLength[i];
    }

    segments = size;
}

void GPSOptions::setValue(QVector3D *target, QVector3D value,
                          int normalize)
{
    float r = value.x();
    float g = value.y();
    float b = value.z();
    if(normalize){
        r /= 255.0f;
        g /= 255.0f;
        b /= 255.0f;
    }

    *target = QVector3D(r, g, b);
}

//void GPSOptions::make_colors_default(){
//    make_default(false);
//}

void GPSOptions::make_default(bool reset_debug){
    this->normalPathColor           = greenColorDefault;
    this->floorLinesColor           = edgeColorDefault;
    this->floorSquareColor          = floorColorDefault;
    this->arrowColor                = arrowColorDefault;
    this->fontColor                 = QVector3D(0.0f, 0.0f, 0.0f);

    this->debugCPointsColor         = QVector3D(0.0f, 0.0f, 0.0f);
    this->debugVoxelPrimaryColor    = QVector3D(0.9f, 0.3f, 0.1f);
    this->debugVoxelNeighboorColor  = QVector3D(0.2f, 0.8f, 0.4f);
    this->debugVoxelContainerColor  = QVector3D(0.0f, 0.0f, 0.7f);
    this->debugVoxelError           = QVector3D(0.7f, 0.0f, 0.0f);

    if(sampleCount < 1){
        sampleCount = 3;
    }

    if(reset_debug){
        reset_debug_vars();
    }
}
