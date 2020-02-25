#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QGeoCoordinate>
#include <QList>
#include <QThread>
#include <QDateTime>
#include <glm/glm.hpp>

#define DATABASE_DRIVER "QSQLITE"
#define DATABASE_SESSIONS_TABLE "Sessions"

struct DBGeoCoordinate{
    QGeoCoordinate coord;
    unsigned char applicationMaskEx[10];
    int segCount;
};

struct DBSession{
    QString tableName;
    QString name;
    QString segments;
    int samples;
    QDateTime qdate;
};

Q_DECLARE_METATYPE(DBGeoCoordinate)
Q_DECLARE_METATYPE(DBSession)
Q_DECLARE_METATYPE(QVector<DBGeoCoordinate>*)
Q_DECLARE_METATYPE(QVector<DBSession>*)

class AssyncManager : public QObject{
    Q_OBJECT
    friend class Database;
private:
    QString DATABASE_TABLE;
    bool opened;
public:
    explicit AssyncManager(){opened = false;}

signals:
    void done();
private slots:
    void insert(QVector<DBGeoCoordinate> *coord);
    void load_all(QVector<DBGeoCoordinate> *coord);
    void initialize(QString path, bool initIfNone);
    void start_session(DBSession session, int *exists);
    void get_sessions(QVector<DBSession> *buffer);
};

class AssyncReceiver : public QObject{
    Q_OBJECT
    friend class Database;
private:
    int userCanUse;
    int inUse;
    QObject *caller;
public:
    explicit AssyncReceiver(){userCanUse = 0;}
private slots:
    void onDone();
};

class Database : public QObject
{
    Q_OBJECT
private:
    static QString DATABASE_NAME;
    static QThread *managerThread;
    static AssyncManager *aManager;
    static AssyncReceiver *aReceiver;
public:
    static QVector<DBSession> get_sessions();
    static void init(QString path, bool initIfNone);
    static int open_session(DBSession session);
//    static void set_database_file(QString path);
    static void insert_gps_coord(DBGeoCoordinate coords);
    static int load_all(QVector<DBGeoCoordinate> *buffer);
    static void initialize();
    static void finalize();

signals:

public slots:
};

#endif // DATABASE_H
