#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickView>
#include "gpsview.h"
#include <QGeoCoordinate>
#include "gpsprovider.h"
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////
#include "gpsfakeprovider.h"
#define TEST_FILE "YOUR_TEST_FILE.txt"
////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){
    QGuiApplication app(argc, argv);
    qmlRegisterType<GPSView>("GPSViewer", 1, 0, "GPSView");
    /**
     * Notes about this format: We don't really need a stencil buffer
     * at least I'm not intending on using one, but it is good practice
     * to have one available. The setSamples line might be dangerous on
     * low hardware without full MultiSampled Framebuffers but it is a simple
     * easy AA without having to write code, in case your hardware complains
     * about this format delete that line.
     */

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);

    /********** D E L E T E   I N   C A S E   O F   P R O B L E M S *******/
//    format.setSamples(8);
    /**********************************************************************/

    format.setSwapInterval(0);

    /**
     * Look Qt is a bit weird, we are going to try to request 3.3 but if it can it
     * might return 4.0+. In any case this is only important so that we find if we are
     * using OpenGL ES 3.0.
     */
    if(QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL){
        qDebug() << "Requesting OpenGL 3.3";
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
    }else{
        qDebug() << "Requesting OpenGL ES 3.0";
        format.setVersion(3, 0);
    }

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setFormat(format);
    view.setSource(QUrl("qrc:///qml/GPSTracking/main.qml"));

    QQuickItem * item = view.rootObject();

    QThread providerThread;
    GPSProvider *provider = new GPSProvider();
    provider->moveToThread(&providerThread);

    /********** R E A D   T H I S *******/
    /**
     * We have a GPSFakeProvider in this code that simulates the behaviour
     * your system should provide, i.e.: giving gps coordinates. This object
     * generates data pre-defined or listen to a specific file for external
     * integration. You should replace this object with your system, it is not
     * meant to be final code, it is only used here so that you understand
     * how to integrate your system.
     */
    QThread fakeProviderThread;
//    GPSFakeProvider *fakeProvider = new GPSFakeProvider(TEST_FILE, 50); // listen to test file
    GPSFakeProvider *fakeProvider = new GPSFakeProvider(50, 1); // generates pre-defined data
    fakeProvider->moveToThread(&fakeProviderThread);

    QObject::connect(item, SIGNAL(interrupt_gps()),
                     fakeProvider, SLOT(interrupt()));

    QObject::connect(fakeProvider, &GPSFakeProvider::newCoordinteArrived,
                     provider, qOverload<QGeoCoordinate>(&GPSProvider::populate),
                     Qt::QueuedConnection);

    ////////////////////////////////////////

    QObject::connect(item, SIGNAL(swap_state_line(int, int)),
                     provider, SLOT(swap_line_state(int, int)));

    QObject::connect(provider, SIGNAL(sectionChanged(int, int)),
                     item, SIGNAL(setRectColor(int, int)));

    providerThread.start();

    /********** R E A D   T H I S *******/
    /**
     * This is a example of how you can setup the database part of this
     * application. The database works with sessions, this means it saves
     * the current work load into a specific table that you need to
     * setup before things get going. It uses these sessions so that the app
     * can provide the ability to reload a previous work.
     * The steps to start a session and the database is as follows:
     *          0 [Optional] - Query for your previous sessions so that you
     *                         see what is saved;
     */
    QVector<DBSession> sessions;
    QMetaObject::invokeMethod(provider, "get_available_sessions",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QVector<DBSession> *, &sessions));

    for(DBSession &session : sessions){
        qDebug() << "-----------------------";
        qDebug() << session.name;
        qDebug() << session.segments;
        qDebug() << session.samples;
    }

    /**
     * Now you can either choose one of the previous sessions the provider
     * found, or you can create a new one like the following:
     *          1 - Setup your session with the specific values you want.
     *              Note that segments variable should be the segments
     *              present in the GPSOptions so that a complete
     *              reload is correct;
     */
    DBSession target;
    target.name = "Felipe"; // be carefull this must be a unique name
    target.qdate = QDateTime::currentDateTime();
    //target.segments = "3.33|3.33|3.33"; // this means 3 segments of size 3.33 m
    target.segments = "";
    for(int i = 0; i < 79; ++i )
        target.segments += "0.2|";

    target.segments += "0.2";

    target.samples = 1;

    /**
     * Start now the database with the specific session, this will take care
     * of opening the session and automatic saving the data as the gps coordinates
     * are provided.
     *          2 - Start the database back-end with your wished session properties.
     *
     * This will start loading any previous saved worked. This process might take a while,
     * if you wish to implement a Loading screen you can make a custom object
     * and connect to the signal 'loadPercetangeChanged' present in the GPSProvider,
     * it is triggered whenever the loading process increases by 1%.
     *
     * QMetaObject::invokeMethod(provider, "init_database", Qt::QueuedConnection,
     *                         Q_ARG(DBSession, target));
     *
     * I moved this bellow so that we can sync the init process with the GPSOptions.
     * You should consider do the same, first call Metrics::initialize and then
     * invoke the init_database function. They kinda need each other for setup.
     */

    ////////////////////////////////////////

    /********** R E A D   T H I S *******/
    /**
     * These variables control overall color and settings
     * for the gps renderer. You can configure the way you wish
     * and then call Metrics::initialize(options)
     * or if you want to dynamically change these values use
     * the call Metrics::update_gps_options(options). Note however
     * than when updating dynamically it is not wise to change the length/samples/segments
     * properties as they are spread across the application and a mismatch might
     * generate incorrect results. The Metrics::update_gps_options is designed
     * for graphical operations only, i.e.: changes in color. Be sure that when
     * the application starts to compute Voxel positions and triangulations
     * the values of length/samples/segments will not change anymore.
     */

    /**
     * Example theme usage: Dark style
     * You can reset colors by invoking the method GPSOptions::make_default
     * on your object. The method GPSOptions::setValue is a simple setter
     * that can be used for copying values. It offers the possibility of
     * normalization, i.e.: dividing values by 255, so you can create colors
     * in the range RGBA [0, 1] or in the range RGB [0, 255]
     */
    GPSOptions options(target);
    GPSOptions::setValue(&options.floorSquareColor,         QVector3D(0.0902f, 0.1490f, 0.2352f));
    GPSOptions::setValue(&options.floorLinesColor,          QVector3D(0.4549f, 0.4978f, 0.3333f));
    GPSOptions::setValue(&options.fontColor,                QVector3D(1.0f, 1.0f, 1.0f));
    GPSOptions::setValue(&options.arrowColor,               QVector3D(19.0f, 181.0f, 199.0f), 1);
    GPSOptions::setValue(&options.debugCPointsColor,        QVector3D(1.0f, 1.0f, 1.0f));
    GPSOptions::setValue(&options.debugVoxelContainerColor, QVector3D(1.0f, 1.0f, 1.0f));
//    options.enableDebugVars = true;

    /* reset ALL options, including debug variables */
//    options.make_default();

    /* reset color options only, preserve debug state */
//    options.make_colors_default();

    /**
     * The function Metrics::initialize must be called at least once otherwise
     * voxel structures won't be instanced, but you can choose when to call, just
     * make sure you call this before any gps data is applied.
     */
    Metrics::initialize(options);
    ////////////////////////////////////////

    QMetaObject::invokeMethod(provider, "init_database", Qt::QueuedConnection,
                              Q_ARG(DBSession, target));

    /**
     * Once initialization is done you can spread the values to your QML
     * so that you can fit the hole theme of the application to these colors.
     */
    QMetaObject::invokeMethod(item, "setLineCount",
                              Q_ARG(QVariant, QVariant::fromValue(options.segments)));
    QMetaObject::invokeMethod(item, "setThemeColors",
                              Q_ARG(QVariant, QVariant::fromValue(options.fontColor)));
    view.show();

    ////////////////////////////////////////
    // this is test code, you should replace this with your own object initialization
    // small sleep to let provider finishes and then start the FakerProvider
    sleep(1);
    qDebug() << "Starting GPSFakeProvider";
    fakeProviderThread.start();
    QMetaObject::invokeMethod(fakeProvider, "provide", Qt::QueuedConnection);
    ////////////////////////////////////////

    return app.exec();
}
