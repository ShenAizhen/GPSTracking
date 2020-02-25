#ifndef GPSVIEW_H
#define GPSVIEW_H

#include <QtQuick/QQuickItem>

#include "gpsrender.h"
#include <QTimer>
#include <QThread>

class GPSView : public QQuickItem
{
    Q_OBJECT

public:
    GPSView(QQuickItem *parent = nullptr);
    ~GPSView();

public slots:
    void sync();
    void cleanup();
    void frame_update();
    void zoom_out_request();
    void zoom_in_request();
    void swap_view_mode();
    void swap_wire_frame_mode();
    void swap_control_point_mode();

//protected:
//    virtual void mouseReleaseEvent(QMouseEvent *event);
//    virtual void mousePressEvent(QMouseEvent *event);
//    virtual void mouseMoveEvent(QMouseEvent *event);

private slots:
    void handleWindowChanged(QQuickWindow *win);
    void onChangedVisible();

private:
    void updateView();

private:
    QPoint previousPosition; //storing previous mouse position to calculate diff
    bool isMouseButtonPressed; //rotating only with mouse pressed
    GPSRenderer *renderer;
    QTimer *fps_timer;
    QThread *fps_thread;
};

#endif // GPSVIEW_H
