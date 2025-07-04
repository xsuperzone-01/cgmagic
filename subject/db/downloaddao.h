#ifndef DOWNLOADDAO_H
#define DOWNLOADDAO_H

#include <QObject>
#include <QSqlQuery>
#include "config/userinfo.h"
#include <QMutexLocker>
#include <QMap>
#include <QVariant>
#include "transfer/transset.h"
#include <QJsonObject>
#include <QListIterator>

#define DDAO DownloadDAO::inst()

class DownloadDAO : public QObject
{
    Q_OBJECT
public:
    static DownloadDAO* inst();
    ~DownloadDAO();
    void releaseDown();

    //file
    db_downfile RfileToDown();
    void UfileIngToWait();
    void Cfile(QList<db_downfile> fL);
    void UfileState(db_downfile &file);
    void UfileDownloadPath(db_downfile file);
    void Dfile(db_downfile &file);
    void Dfiles(QList<db_downfile> fileL);
    void DErrorFile();
    QList<db_downfile> RallFile();
    void changeSearch(QString search);

    void Cunity(QString order, QString downPath);
    QString RunityPath(QString order);
    void Uu3dOk(QString order);
    void Uu3dSum(QString order, int sum);
    QString Ru3dFinish();

    void CResultDir(QString order, QString downPath);
    QString ResultDir(QString order);
    void DelResultDir(QString order);
private:
    DownloadDAO(QObject *parent = 0);
    void createTable();
    void fileDB2Class(db_downfile& f, QSqlQuery& query);
    QList<db_downfile> allPrepare();
signals:

    void allFile(QList<db_downfile>);
    void fileSpeedChanged(QString orderNum, int id, qint64 downSize, qint64 size, qint64 speed);
    void fileStateChanged(db_downfile file);
    void fileDeleted(db_downfile);

    void refreshAll();
private:
    static DownloadDAO* m_d;

    QSqlQuery m_fileQ;
    QMutex m_fileM;

    QSqlDatabase m_db;

    int m_fileC;
    QString m_search;
};

#endif // DOWNLOADDAO_H
