#ifndef GPSFAKEPROVIDER_H
#define GPSFAKEPROVIDER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QGeoCoordinate>

typedef enum{
    PASSIVE=0, ACTIVE=1
}Method;

class GPSFakeProvider : public QObject
{
    Q_OBJECT
public:
    explicit GPSFakeProvider(const char *binding_path, int query_interval,
                             QObject *parent = nullptr);
    explicit GPSFakeProvider(int trigger_timeout, int count_per_transfer,
                             QObject *parent = nullptr);

signals:
    void newCoordinteArrived(QGeoCoordinate coord);

public slots:
    void provide();
    void onQueryTimeout();
    void interrupt();

private:
    void populate();
    void internal_init();
    void passive_generation();
    void active_generation();
    void fill_ptr_data();

    QVector<QGeoCoordinate> *dataPtr;
    QVector<QGeoCoordinate> *innerPtr;
    QMutex *syncMutex;
    std::string queryFile;
    QTimer *queryTimer;
    int queryInterval;
    int initialized;
    int hPtr;
    int transferCount;
    Method method;
};

#endif // GPSFAKEPROVIDER_H
