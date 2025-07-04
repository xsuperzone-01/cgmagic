#ifndef UPLOADDAO_H
#define UPLOADDAO_H

#include <QSqlQuery>
#include <QMutexLocker>
#include <QVariant>
#include <QSqlError>
#include <QThread>
#include "transfer/transset.h"
#include <QJsonObject>

#define UPDAO UploadDAO::inst()

class UploadDAO : public QObject
{
    Q_OBJECT
public:
    static UploadDAO* inst();
    ~UploadDAO();
    void releaseUp();

    //file
    void Cfile(db_upfile tmpUp, QList<db_upfile> fL);
    void UfileId(db_upfile &file);
    db_upfile RfileToUp();
    void UfileState(db_upfile& file);
    void Dfile(db_upfile& file);
    void DfileByOrder(QString order);
    void DErrorFile(QString order = "");
    QList<db_upfile> RallFile();
    QList<db_upfile> RallFileExceptError();
    QList<db_upfile> RallFileError();
    void UfileIngToWait();
    void UfileModTime(db_upfile &file);
    void UfileHash(db_upfile& file);

    void CsrcPath(QString order, QString path);
private:
    UploadDAO();
    void createTable();
    void fileDB2Class(db_upfile& f, QSqlQuery& query);
    QList<db_upfile> allPrepare();
signals:

    void allFile(QList<db_upfile> fl);
    void fileDeleted(db_upfile file);
    void fileDeletedByOrder(QString order);
    void fileStateChanged(db_upfile file);
    void fileSpeedChanged(QString order, QString lpath, qint64 upped, qint64 size, qint64 speed);

    void refreshAll();
private:
    static UploadDAO* m_d;

    QMutex m_fileM;
    QSqlQuery m_fileQ;

    QSqlDatabase m_db;

    int m_fileC;
};

#endif // UPLOADDAO_H
