#include "database.h"
#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QByteArray>

//#define DISABLE_LOAD

#define DATBASE_BUFFER              1
#define DATABASE_TIMESTAMP_FMT      "dd mm yyyy hh:mm:ss"

/* Checks for existence of the table Location, inspect Sqlite3 master component and check for our table */
static const char * SQL_EXISTS_RULE = "SELECT count(*) FROM sqlite_master WHERE type = \'table\' AND name = \'%1\';";

/* Creates the standard table we are going to use, has an autoincrement ID */
static const char * SQL_CREATE_DATA_RULE = "CREATE TABLE %1(ID INTEGER PRIMARY KEY AUTOINCREMENT, Latitude REAL, Longitude REAL,AppMaskEx TEXT);";

static const char * SQL_CREATE_SESSION_RULE = "CREATE TABLE %1(ID INTEGER PRIMARY KEY AUTOINCREMENT, Segments TEXT, Samples INTEGER, Date TEXT, Name TEXT);";

static const char * SQL_INSERT_SESSION_RULE = "INSERT INTO %1 (Segments, Samples, Date, Name) VALUES (\'%2\', %3, \'%4\', \'%5\');";

static const char * SQL_SESSION_EXISTS_RULE = "SELECT count(*) FROM %1 WHERE Name = \'%2\';";

static const char * SQL_SELECT_ALL_RULE = "SELECT * from %1 ORDER BY ID;";

QString Database::DATABASE_NAME("database.db");
QThread *Database::managerThread = nullptr;
AssyncManager *Database::aManager = nullptr;
AssyncReceiver *Database::aReceiver = nullptr;

static QVector<DBGeoCoordinate> sector0;
static QVector<DBGeoCoordinate> sector1;
static bool mInited = false;
static QSqlDatabase mDatabase;
static bool mOk = false;
static QMutex *dbMutex = nullptr;
static bool managerBusy = false;

static QSqlQuery internal_exec_query(QString str_query){
    QSqlQuery query;
    if(mOk){
        query.prepare(str_query);
        if(!query.exec()){
            QSqlError error = query.lastError();
            qDebug() << "Failed to execute " << error.text() << " " << str_query;
        }
    }else{
        qDebug() << "Requested query when database was not correctly setup";
    }

    return query;
}

static void init_manager(QString path, bool initIfNone, QString sessionTable){
    if(!mInited){
        mOk = true;
        if(!QSqlDatabase::isDriverAvailable(DATABASE_DRIVER)){
            qDebug() << "No available driver support for " << DATABASE_DRIVER;
            mOk = false;
        }

        if(mOk){
            mDatabase = QSqlDatabase::addDatabase(DATABASE_DRIVER);
            mDatabase.setDatabaseName(path);
            mOk = mDatabase.open();

            if(initIfNone){
                QString rule(SQL_EXISTS_RULE);
                QString str_query = rule.arg(sessionTable);
                QSqlQuery query(mDatabase);
                query = internal_exec_query(str_query);
                if(query.first()){
                    int val = query.value(0).toInt();
                    if(val == 0){
                        qDebug() << "Creating database";
                        str_query = QString(SQL_CREATE_SESSION_RULE);
                        str_query = str_query.arg(sessionTable);
                        Q_UNUSED(internal_exec_query(str_query));
                    }
                }
            }

            mInited = true;
            qDebug() << "Database initialized";
        }
    }

    if(!dbMutex){
        dbMutex = new QMutex();
    }
}

void AssyncManager::initialize(QString path, bool initIfNone){
    init_manager(path, initIfNone, DATABASE_SESSIONS_TABLE);
}

QString mask2string(unsigned char *mask,int segCount)
{
    unsigned char maskEx[100]="";
    for(int i = 0 ; i < segCount ; ++i)
    {
        int index = i / 8;
        int num = i % 8;
        maskEx[i] = (mask[index] >> num) & 1U ;
    }

    QString str="";
    for(int i = 0; i < segCount ; i++ )
        str += QString::number( maskEx[i]);

    return str;
}
//rev
int string2mask(QString str,unsigned char *mask)
{
    for(int i = 0; i < str.size() ; i++ )
        mask[i] = (unsigned char)QString(str.at(i)).toUInt();

    return str.size();
}

void AssyncManager::insert(QVector<DBGeoCoordinate> *buffer){
    if(opened){
        QMutexLocker locker(dbMutex);
        QString str_query("INSERT INTO ");
        str_query += QString(DATABASE_TABLE) + QString("(Latitude, Longitude,AppMaskEx) VALUES ");
        for(int i = 0; i < buffer->size(); i += 1){
            DBGeoCoordinate coords = buffer->at(i);
            QString lat  = QString::number(coords.coord.latitude(), 'f', 9);
            QString lon  = QString::number(coords.coord.longitude(), 'f', 9);
            QString maskEx = mask2string(coords.applicationMaskEx,coords.segCount);//rev

            str_query += QString("(") + lat + QString(",") +
                         lon + /*QString(",") + mask +*/ QString(",'") + maskEx +QString("')");
            if(i < buffer->size() - 1){
                str_query += QString(", ");
            }else{
                str_query += QString(";");
            }
        }

        internal_exec_query(str_query);

        buffer->clear();
    }else{
        qDebug() << "Requested insertion without session opened";
    }

    emit done();
}

void AssyncManager::get_sessions(QVector<DBSession> *buffer){
    if(buffer){
        QMutexLocker locker(dbMutex);
        QString str_query(SQL_SELECT_ALL_RULE);
        str_query = str_query.arg(DATABASE_SESSIONS_TABLE);
        QSqlQuery query = internal_exec_query(str_query);
        while(query.next()){
            DBSession session;
            session.segments          = query.value(1).toString();
            session.samples           = query.value(2).toInt();
            session.qdate             = query.value(3).toDateTime();
            session.name              = query.value(4).toString();
            buffer->push_back(session);
        }
    }
}

void AssyncManager::start_session(DBSession session, int *exists){
    QMutexLocker locker(dbMutex);
    bool create = true;
    QString str_query(SQL_SESSION_EXISTS_RULE);
    str_query = str_query.arg(DATABASE_SESSIONS_TABLE).arg(session.name);
    QSqlQuery query = internal_exec_query(str_query);
    *exists = 0;
    DATABASE_TABLE = session.name;
    if(query.next()){ // this session already exists just get a handle to it
        int r = query.value(0).toInt();
        if(r != 0){
            *exists = 1;
            create = false;
        }
    }

    if(create){
        str_query = QString(SQL_CREATE_DATA_RULE);
        str_query = str_query.arg(session.name);
        Q_UNUSED(internal_exec_query(str_query));

        str_query = QString(SQL_INSERT_SESSION_RULE);
        QString date = session.qdate.toString(DATABASE_TIMESTAMP_FMT);
        str_query = str_query.arg(DATABASE_SESSIONS_TABLE).arg(session.segments)
                    .arg(session.samples).arg(date).arg(session.name);
        Q_UNUSED(internal_exec_query(str_query));
    }

    opened = true;
}

void AssyncManager::load_all(QVector<DBGeoCoordinate> *buffer){
#ifndef DISABLE_LOAD
    if(buffer && opened){
        QMutexLocker locker(dbMutex);
        buffer->clear();
        QString str_query(SQL_SELECT_ALL_RULE);
        str_query = str_query.arg(DATABASE_TABLE);
        QSqlQuery query = internal_exec_query(str_query);
        qreal lat, lon;
        int mask;
        bool r0Ok, r1Ok, uiOk;
        unsigned char maskEx[100];//rev
        while(query.next()){
            lat = query.value(1).toDouble(&r0Ok);
            lon = query.value(2).toDouble(&r1Ok);
            mask = query.value(3).toInt(&uiOk);
            int segCount = string2mask(query.value(4).toString(),maskEx);//rev

            if(r0Ok && r1Ok && uiOk){
                DBGeoCoordinate coord;
                coord.coord = QGeoCoordinate(lat,lon);
                //coord.applicationMask = (unsigned int)mask;
                coord.segCount = segCount;

                memset(coord.applicationMaskEx,10, 1);
                for(int i = 0 ; i < segCount ; ++i)
                {
                    int index = segCount / 8;
                    int num = segCount % 8;
                    coord.applicationMaskEx[index] |= (maskEx[i] << num);
                }


                buffer->push_back(coord);
            }
        }
    }else if(!opened){
        qDebug() << "Requested data load without session opened";
    }
#else
    qDebug() << "Warning: Skipping database load";
#endif
}

void AssyncReceiver::onDone(){
    QVector<DBGeoCoordinate>*ptr = &sector0;
    if(userCanUse != 0){
        ptr = &sector1;
    }

    if(ptr->size() > DATBASE_BUFFER){
        QMetaObject::invokeMethod(caller, "insert",
                                  Qt::QueuedConnection,
                                  Q_ARG(QVector<DBGeoCoordinate>*, ptr));
        managerBusy = true;
        inUse = userCanUse;
        userCanUse = 1 - userCanUse;
    }else{
        managerBusy = false;
    }
}

void Database::init(QString path, bool initIfNone){
    if(!aManager){
        aManager = new AssyncManager();
        aReceiver = new AssyncReceiver();
        aReceiver->caller = aManager;
        managerThread = new QThread();
        aManager->moveToThread(managerThread);

        QObject::connect(aManager, SIGNAL(done()),
                         aReceiver, SLOT(onDone()),
                         Qt::QueuedConnection);

        managerThread->start();
        QMetaObject::invokeMethod(aManager, "initialize",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, path),
                                  Q_ARG(bool, initIfNone));
    }
}

//void Database::set_database_file(QString path){
//    DATABASE_NAME = path;
//}

int Database::open_session(DBSession session){
    init(DATABASE_NAME, true);
    int rv = 0;
    QMetaObject::invokeMethod(aManager, "start_session",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(DBSession, session),
                              Q_ARG(int *, &rv));
    return rv;
}

/*
 * We can probably load faster by getting a pointer to a yield callback
*/
int Database::load_all(QVector<DBGeoCoordinate> *buffer){
    init(DATABASE_NAME, true);
    int elements = 0;
    QMetaObject::invokeMethod(aManager, "load_all",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QVector<DBGeoCoordinate>*, buffer));
    elements = buffer->size();
    return elements > 0 ? 1 : 0;
}

void Database::initialize(){
    init(DATABASE_NAME, true);
}

void Database::finalize()
{
    mDatabase.close();
    mInited = false;

    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

QVector<DBSession> Database::get_sessions(){
    init(DATABASE_NAME, true);
    QVector<DBSession> sessions;
    QMetaObject::invokeMethod(aManager, "get_sessions",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QVector<DBSession> *, &sessions));
    return sessions;
}

void Database::insert_gps_coord(DBGeoCoordinate coords){
    init(DATABASE_NAME, true);
    QVector<DBGeoCoordinate>*ptr = &sector0;
    if(aReceiver->userCanUse != 0){
        ptr = &sector1;
    }

    if(managerBusy){
        ptr->push_back(coords);
    }else{
        ptr->push_back(coords);
        if(ptr->size() > DATBASE_BUFFER){
            managerBusy = true;
            aReceiver->inUse = aReceiver->userCanUse;
            aReceiver->userCanUse = 1 - aReceiver->userCanUse;
            QMetaObject::invokeMethod(aManager, "insert",
                                      Qt::QueuedConnection,
                                      Q_ARG(QVector<DBGeoCoordinate>*, ptr));
        }
    }
}
