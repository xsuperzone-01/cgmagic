#include "userconfig.h"
#include "config/userinfo.h"
#include <qDebug>
#include <QSqlError>
userConfig* userConfig::m_d = NULL;

userConfig::userConfig(QObject *parent) :
    QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "db_Config");
    m_db.setDatabaseName(USERINFO->getUserPath() + "/Config.db");
    m_db.open();

    qDebug() << "new userConfig, userId:" << USERINFO->userId();
    m_userConfig = QSqlQuery(m_db);
    createTable();
}

userConfig *userConfig::inst()
{
    if (m_d == NULL) {
        m_d = new userConfig;
    }
    return m_d;
}

void userConfig::release()
{
    qDebug() << __FUNCTION__;
    m_db.close();
    if (m_d) {
        m_d->deleteLater(); m_d = NULL;
    }
}

void userConfig::createTable()
{
    QString createConfig = "create table config(userid int,ResultDir varchar,AutoSendResult int,ResultDirType varchar,ProjectManager varchar,ProjectManager2 varchar,latest varchar,PopupDate varchar,CustomProjectEn int)";
    m_userConfig.exec(createConfig);

    m_userConfig.prepare(QString("alter table config add configFile varchar"));
    m_userConfig.exec();
    m_userConfig.prepare(QString("alter table config add rbAdvert varchar"));
    m_userConfig.exec();
    m_userConfig.prepare(QString("alter table config add psd bool"));
    m_userConfig.exec();
    m_userConfig.prepare(QString("alter table config add downloadNoCover bool"));
    m_userConfig.exec();
    m_userConfig.prepare(QString("alter table config add vrmode int"));
    m_userConfig.exec();
    m_userConfig.prepare(QString("alter table config add vrver int"));
    m_userConfig.exec();
    m_userConfig.prepare(QString("alter table config add vrvers varchar"));
    m_userConfig.exec();

    m_userConfig.prepare("select * from config where userid = ?");
    m_userConfig.addBindValue(USERINFO->userId());
    m_userConfig.exec();
    if (!m_userConfig.next())
    {
        m_userConfig.prepare("insert into config (userid) values (?)");
        m_userConfig.addBindValue(USERINFO->userId());
        m_userConfig.exec();
    }

    m_userConfig.exec("create table sp(software varchar,sel int,sp int)");
}

QVariant userConfig::readUserConfig(QString key)
{
    QMutexLocker locker(&m_fileM);
    m_userConfig.prepare("select * from config where userid = ?");
    m_userConfig.addBindValue(USERINFO->userId());
    m_userConfig.exec();

    QVariant value;
    while (m_userConfig.next()) {
        value = m_userConfig.value(key);
    }
    return value;
}

void userConfig::saveConfig(QString key, QVariant value)
{
    QMutexLocker locker(&m_fileM);
    m_userConfig.prepare(QString("alter table config add %1 %2").arg(key).arg(value.type() == QVariant::Int? "int" : "varchar"));
    m_userConfig.exec();

    m_userConfig.prepare(QString("update config set %1 = ? where userid = ?").arg(key));
    m_userConfig.addBindValue(value);
    m_userConfig.addBindValue(USERINFO->userId());
    m_userConfig.exec();
}

db_sp userConfig::sp(QString software)
{
    software = software.toLower();

    m_userConfig.prepare("select * from sp where software = ?");
    m_userConfig.addBindValue(software);
    m_userConfig.exec();

    db_sp ret;
    while (m_userConfig.next()) {
        ret.software = software;
        ret.select = m_userConfig.value("sel").toInt();
        ret.sp = m_userConfig.value("sp").toInt();
    }
    return ret;
}

void userConfig::setSp(db_sp sp)
{
    sp.software = sp.software.toLower();

    m_userConfig.prepare("delete from sp where software = ?");
    m_userConfig.addBindValue(sp.software);
    m_userConfig.exec();

    m_userConfig.prepare("insert into sp (software,sel,sp) values (?,?,?)");
    m_userConfig.addBindValue(sp.software);
    m_userConfig.addBindValue(sp.select);
    m_userConfig.addBindValue(sp.sp);
    m_userConfig.exec();
}
