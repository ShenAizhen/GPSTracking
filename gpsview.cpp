#include "gpsview.h"
#include <QtQuick/QQuickWindow>
#include <QDebug>
#include <QMetaObject>

GPSView::GPSView(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
//    setAntialiasing(true);

    renderer = nullptr;
    isMouseButtonPressed = false;

    float time_es = 1000.0f / static_cast<float>(TARGET_FPS);
    fps_thread = new QThread();
    fps_timer = new QTimer(nullptr);
    fps_timer->setInterval(static_cast<int>(time_es));
    fps_timer->moveToThread(fps_thread);
    QObject::connect(fps_timer, SIGNAL(timeout()), this, SLOT(frame_update()),
                     Qt::QueuedConnection);

//    setAcceptHoverEvents(true);
//    setAcceptedMouseButtons(Qt::AllButtons);
//    setFlag(ItemAcceptsInputMethod, true);

    connect(this, SIGNAL(windowChanged(QQuickWindow*)),
            this, SLOT(handleWindowChanged(QQuickWindow*)));

    connect(this, SIGNAL(visibleChanged()), this, SLOT(onChangedVisible()));

    fps_thread->start();
    QMetaObject::invokeMethod(fps_timer, "start", Qt::QueuedConnection);
}

GPSView::~GPSView(){
    delete renderer;
}

void GPSView::swap_view_mode(){
    renderer->swap_view_mode();
}

void GPSView::sync(){
    if(!renderer){
        renderer = new GPSRenderer();
        renderer->setVisible(this->isVisible());
        connect(window(), SIGNAL(beforeRendering()),
                renderer, SLOT(render()), Qt::DirectConnection);
    }

    renderer->setViewportSize(QSize(static_cast<int>(width()), static_cast<int>(height())));
    renderer->setViewportPoint(QPoint(static_cast<int>(x()), static_cast<int>(y())));
    renderer->render();
}

void GPSView::swap_control_point_mode(){
    GPSOptions options = Metrics::get_gps_option();
    options.renderControlPoints = !options.renderControlPoints;
    Metrics::update_gps_options(options);
}

void GPSView::swap_wire_frame_mode(){
    GPSOptions options = Metrics::get_gps_option();
    options.renderWireframe = !options.renderWireframe;
    Metrics::update_gps_options(options);
}

void GPSView::onChangedVisible(){
    if(renderer)
        renderer->setVisible(this->isVisible());
}

void GPSView::frame_update(){
    this->setFlag(QQuickItem::ItemHasContents, true);
    this->update();
    if(window()){
        window()->update();
    }
}

void GPSView::zoom_out_request(){
    renderer->zoom_out();
}

void GPSView::zoom_in_request(){
    renderer->zoom_in();
}

void GPSView::cleanup(){
    if (renderer) {
        delete renderer;
        renderer = nullptr;
    }
}

void GPSView::handleWindowChanged(QQuickWindow *wnd){
    if (wnd) {
        connect(wnd, SIGNAL(beforeSynchronizing()), this, SLOT(sync()), Qt::DirectConnection);
        connect(wnd, SIGNAL(sceneGraphInvalidated()), this, SLOT(cleanup()), Qt::DirectConnection);
        wnd->setClearBeforeRendering(false);
    }
}

//void GPSView::mousePressEvent(QMouseEvent *event){
//    QQuickItem::mousePressEvent(event);
//    qDebug() << "mousePressEvent";
//    if (event->buttons() == Qt::LeftButton) {
//        previousPosition = event->pos();
//        isMouseButtonPressed = true;
//        event->accept();
//    }
//}

//void GPSView::mouseReleaseEvent(QMouseEvent *event){
//    QQuickItem::mouseReleaseEvent(event);
//    qDebug() << "mouseReleaseEvent";
//    if (event->buttons() == Qt::LeftButton) {
//        isMouseButtonPressed = false;
//        event->accept();
//    }
//}

//void GPSView::mouseMoveEvent(QMouseEvent *event){
//    QQuickItem::mouseMoveEvent(event);
//    if (isMouseButtonPressed) {
//        QPoint cp = event->pos();
////        int cx = cp.x();
////        int px = previousPosition.x();

//        //qDebug() << (cx - px);
//        updateView();

//        previousPosition = cp;
//    }
//}

void GPSView::updateView(){
    renderer->render();
    if (window()) {
        window()->update();
    }
}
