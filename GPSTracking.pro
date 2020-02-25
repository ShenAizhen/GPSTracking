# Add more folders to ship with the application, here
folder_01.source = qml/GPSTracking
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01
QT += qml quick positioning svg sql
INCLUDEPATH += $$PWD/qml
DEPENDPATH += $$PWD
# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# Remove this to not use modern OpenGL
DEFINES += HAVE_GL33
win32:LIBS += -lopengl32

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    gpsoptions.cpp \
    gpsprovider.cpp \
    gpsrender.cpp \
    graphics.cpp \
    gpsview.cpp \
    view.cpp \
    pathing.cpp \
    database.cpp \
    gpsfakeprovider.cpp

RESOURCES = application.qrc

# Default rules for deployment.
TARGET = navigation
unix: target.path = /navigation/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    bits.h \
    gpsoptions.h \
    gpsprovider.h \
    gpsrender.h \
    graphics.h \
    gpsview.h \
    polyline2d/include/LineSegment.h \
    polyline2d/include/Polyline2D.h \
    polyline2d/include/Vec2.h \
    view.h \
    common.h \
    pathing.h \
    database.h \
    voxel2d.h \
    gpsfakeprovider.h

DISTFILES += \
    qml/GPSTracking/Velocimeter.qml \
    qml/GPSTracking/main.qml \
    qml/images/nav_adjusts_80x80.svg \
    qml/images/nav_arrow_80x80.svg \
    qml/images/nav_zoom-in_80x80.svg \
    qml/images/nav_zoom-out_80x80.svg \
    qml/GPSTracking/SetButton.qml \
    shaders/pathing2_frag.glsl \
    shaders/pathing2_vertex.glsl \
    shaders/pathing_frag.glsl \
    shaders/pathing_vertex.glsl \
    shaders/segment_vertex.glsl \
    shaders/segment_frag.glsl \
    shaders/instances_vertex.glsl \
    shaders/instances_frag.glsl
