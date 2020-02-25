#ifndef GPSPROVIDER_H
#define GPSPROVIDER_H

#include <QObject>
#include <QGeoCoordinate>
#include <QList>
#include <QTimer>
#include <QMutex>
#include <string>
#include <common.h>
#include "database.h"

class GPSProvider : public QObject
{
    Q_OBJECT
public:
    GPSProvider();
    void populate(QGeoCoordinate coord);

signals:
//    void loadPercetangeChanged(int percentage, int done);
//    void hitStatusChanged(uchar *hitMask);
    void sectionChanged(int index, int value);

public slots:
    void swap_line_state(uchar linesMask);
    void swap_line_state(int line, int state);

    /*
     * You are supposed to call this function using QMetaObject
     * and not a direct call. Remenber this is a thread.
    */
    void get_available_sessions(QVector<DBSession> *sessions);
    void init_database(DBSession session);
    void finalize_database();

private:
    unsigned char currentHitMaskEx[MAX_MASK_SEG];
    bool initedDb;
    QGeoCoordinate pre;

    void internal_init(DBSession session);
    void internal_update(QGeoCoordinate &coord);

};

#endif // GPSPROVIDER_H
