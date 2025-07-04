#include "clearmax.h"

#include "db/uploaddao.h"
#include "config/userinfo.h"
#include "tool/xfunc.h"
#include <QDirIterator>
#include <QMutex>
#include <QMutexLocker>

QMutex mutex;  //即使new多次，并不影响; 全局变量的生命周期只会有一个实例，即指向同一个内存地址

ClearMax::ClearMax(QObject *parent) :
    QThread(parent)
{

}

void ClearMax::run()
{
    QMutexLocker locker(&mutex); //局部变量QMutexLocker等函数执行完后才会销毁，并释放锁mutex；第二个程序才可以使用；
    qDebug()<<"enter mutex mode";
    QStringList clearDbL;
    QStringList fileL;

    QSqlDatabase maxDb = QSqlDatabase::addDatabase("QSQLITE", "db_clearMax");

    QDirIterator d(USERINFO->appDataPath(), QStringList()<<"fileUpload.db", QDir::Files, QDirIterator::Subdirectories);
    while (d.hasNext()) {
        d.next();
        QFileInfo info = d.fileInfo();
        maxDb.setDatabaseName(info.absoluteFilePath());
        if (maxDb.open()) {
            QSqlQuery query = QSqlQuery(maxDb);

            QStringList orderL;
            QStringList pathL;
            query.prepare("select * from upsrc");
            query.exec();
            while (query.next()) {
                orderL << query.value("ordernum").toString();
                pathL << query.value("lpath").toString();
            }

            QStringList delL;
            for (int i = 0; i < orderL.length(); ++i) {
                query.prepare("select count(lpath) from up where ordernum = ?");
                query.addBindValue(orderL.at(i));
                query.exec();
                while (query.next()) {
                    int count = query.value(0).toInt();
                    if (0 == count) {
                        if (!pathL.at(i).endsWith("Local")) {
                            XFunc::veryDel(pathL.at(i));
                            qDebug()<< "delete:"<<pathL.at(i);
                        }
                        delL << orderL.at(i);
                    }
                }
            }
            for (int i = 0; i < delL.length(); ++i) {
                query.prepare("delete from upsrc where ordernum = ?");
                query.addBindValue(delL.at(i));
                query.exec();
            }
            maxDb.close();
        }
    }
    qDebug()<<"quit mutex mode";
}
