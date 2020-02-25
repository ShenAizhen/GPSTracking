#include "gpsprovider.h"
#include <QThread>
#include <QtDebug>
#include <sys/file.h>
#include <unistd.h>
#include <cstdio>
#include "graphics.h"

#define PRINT_REAL_VALUE(coord)  QString("%1").arg(coord, 0, 'g', 14)

GPSProvider::GPSProvider(){
    //currentHitMask = 0x00;
    initedDb = false;

    memset(currentHitMaskEx,0,MAX_MASK_SEG);
}

void GPSProvider::internal_update(QGeoCoordinate &coord) {
    if (coord.isValid())
    {
//        if (coord.longitude() > 0.0) // this is wrong
        /* if it's positive, it's, in truth, negative, and vice-and-versa.
         * Don't know why this app has "mirrored" the coordinates.
         */
        coord.setLongitude( -coord.longitude() );

        if (coord.distanceTo(pre) > 10.0)
        {
            memset(currentHitMaskEx, 0, MAX_MASK_SEG);
            qDebug() << "This is another lap!";

            // DEBUG
            qDebug() << "Previous position:" << PRINT_REAL_VALUE(pre.longitude())
                                      << ":" << PRINT_REAL_VALUE(pre.latitude());
            qDebug() << "Current position:"  << PRINT_REAL_VALUE(coord.longitude())
                                      << ":" << PRINT_REAL_VALUE(coord.latitude());
            qDebug() << "Distance:" << PRINT_REAL_VALUE(coord.distanceTo(pre));
            // -----

            Metrics::initialize();
        }
        pre = coord;

        Metrics::lock_for_series_update();

        int changed         = 0;
        update_data out     = Metrics::update_orientation(coord, changed);
        int segments        = out.segments;
        uchar *hitMaskEx    = out.currentHitMaskEx;

        // if there's a change in bitmask, will emit signal with it
        if (memcmp(hitMaskEx, currentHitMaskEx, MAX_MASK_SEG)) {

            //** [SET SECTION STATUS TO GRID IN FRONTEND] **//
            ///////////
            quint8 maxByte = static_cast<quint8>(((segments - 1) / 8) + 1);
            for (quint8 byteI = 0; byteI < maxByte; ++byteI) {
                if (hitMaskEx[byteI] != currentHitMaskEx[byteI]) {
                    quint8 tempMask = 1;
                    for (quint8 bitI = 8; bitI > 0; --bitI) {
                        quint8 oldbit = (hitMaskEx[byteI] & tempMask) > 0 ? 1 : 0;
                        quint8 newbit = (currentHitMaskEx[byteI] & tempMask) > 0 ? 1 : 0;

                        tempMask = static_cast<quint8>(tempMask << 1);

                        if (oldbit != newbit) {
                            int index = ((byteI + 1) * 8) - bitI;
                            emit sectionChanged(index, newbit);
                        }
                    }
                }
            }
            ///////////
            //!! [SET SECTION STATUS TO GRID IN FRONTEND] **//


            memcpy(currentHitMaskEx, hitMaskEx, MAX_MASK_SEG);
//            emit hitStatusChanged(hitMaskEx);
        }

        if (changed) {
            Orientation_t curr = Metrics::get_orientation_unsafe();
            DBGeoCoordinate currentLocation;
            currentLocation.coord = curr.geoCoord;
            currentLocation.segCount = segments;
            memcpy(currentLocation.applicationMaskEx, out.movementMaskEx, MAX_MASK_SEG);

            if (initedDb)
                Database::insert_gps_coord(currentLocation);
        }
    }

    Metrics::unlock_series_update();
}
void GPSProvider::populate(QGeoCoordinate coord)
{
    internal_update(coord);
}

void GPSProvider::swap_line_state(uchar linesMask)
{
    Metrics::set_line_state(&linesMask);
}

void GPSProvider::swap_line_state(int line, int state)
{
    Metrics::set_line_state(line, state);
}

void GPSProvider::init_database(DBSession session){
    if(!initedDb){
       Database::initialize();
       initedDb = true;
    }

    internal_init(session);
}

void GPSProvider::finalize_database()
{
    initedDb = false;
//    Database::finalize();
}

void GPSProvider::get_available_sessions(QVector<DBSession> *sessions){
    if(!initedDb){
       Database::initialize();
       initedDb = true;
    }

    if(sessions){
        *sessions = Database::get_sessions();
    }
}

void GPSProvider::internal_init(DBSession target){
    int exists = Database::open_session(target);
    Metrics::load_start();
    if(exists){
        qDebug() << "Checking for previous data";
        QVector<DBGeoCoordinate> buffer;
        Database::load_all(&buffer);
        int total = buffer.size();
        float ftotal = SCAST(float, total);
        float inserted = 0.0f;
        float oldPct = 0.0f;
        float curPct = 0.0f;
        int percentage = 0;

        if(total > 0){
            for(DBGeoCoordinate &coord : buffer){
                QGeoCoordinate qcoord = coord.coord;
                unsigned char *maskEx = coord.applicationMaskEx;//rev
                Metrics::load_updateEx(qcoord, maskEx);//rev

                inserted += 1.0f;
                curPct = (inserted / ftotal) * 100.0f;
                if(curPct - oldPct > 1 && SCAST(int, inserted) != total){
                    oldPct = curPct;
                    percentage = SCAST(int, std::floor(curPct));
//                    emit loadPercetangeChanged(percentage, 0);
                }
            }
        }
    }

//    qDebug() << "Loaded 100 %";
//    emit loadPercetangeChanged(100, 1);

    Metrics::load_finish();
}
