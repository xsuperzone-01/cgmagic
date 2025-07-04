#ifndef USERCONFIG_H
#define USERCONFIG_H

#include <QObject>
#include <QVariant>
#include <QSqlQuery>
#include <QMutex>
class db_userConfig{
public:
    int userid;
    QString ResultDir;
    int AutoSendResult;
    QString ResultDirType;
    QString ProjectManager;
    QString ProjectManager2;
    QString latest;
    QString PopupDate;
    int CustomProjectEn;

    db_userConfig() {
       userid = 0;
       ResultDir = "";
       AutoSendResult = 1;
       ResultDirType = "ResultDir";
       ProjectManager = "";
       ProjectManager2 = "";
       latest = "";
       PopupDate = "";
       CustomProjectEn = 0;
    }
};

class db_sp {
public:
    QString software;
    int select; //0 默认 1 禁用 2 启用
    int sp; //0 没有sp 1 sp1 2 sp2  范围1-8 默认1

    db_sp() {
        select = 0;
        sp = 1;
    }

    bool valid() {
        return !software.isEmpty();
    }

    int validSp() {
        if (sp < 1 || sp > 8)
            return 1;
        return sp;
    }

    int getSp() {
        if (1 == select)
            return 0;
        return validSp();
    }
};

class userConfig : public QObject
{
    Q_OBJECT
public:
    static userConfig* inst();
    void release();
    QVariant readUserConfig(QString key);
    void saveConfig(QString key, QVariant value);

    db_sp sp(QString software);
    void setSp(db_sp sp);

private:
    explicit userConfig(QObject *parent = 0);
    void createTable();
private:
    static userConfig* m_d;
    QMutex m_fileM;
    QSqlQuery m_userConfig;
    QSqlDatabase m_db;
};

#endif // USERCONFIG_H
