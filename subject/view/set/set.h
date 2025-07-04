#ifndef SET_H
#define SET_H

#include <QWidget>
#include <QSettings>
#include <QPointer>

namespace Ui {
class Set;
}

class Set : public QWidget
{
    Q_OBJECT

public:
    explicit Set(QWidget *parent = nullptr);
    ~Set();

    void initSet();
    static void initSetIni();

    static QString changeLan();
    static void setAutoRun(bool v);
    static bool updateStableVersion();
    static void setUpdateStableVersion(bool v);
    static bool loginHide();
    static void setLoginHide2(bool v);
    static bool loginSilent();
    static void setLoginSilent(bool v);
    static void setAutoLogin(bool v);
    static QString cacheDir();
    static void setCacheDir(QString v);


    int Langnum();

private slots:
    void on_openCacheDir_clicked();
    void setLoginHide(bool v);
    void setLang();
    void setCache();

private:
    Ui::Set *ui;
    static bool m_autoLogin;
    static QString m_cacheDir;
};

#endif // SET_H
