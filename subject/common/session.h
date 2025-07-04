#ifndef SESSION_H
#define SESSION_H

#include <QPointer>
#include <QString>
#include <QTextEdit>

#include <tool/singleapp.h>
#include "common/Singleton.h"

#include "view/mainwindow.h"
#include "view/login.h"
#include "transfer/up/uphandler.h"
#include "transfer/down/downhandler.h"

class Session : public QObject
{
    Q_OBJECT
    PATTERN_SINGLETON_DECLARE(Session);
public:
    QString globalStyle(int x = 0);
    QString ScaleSheet(QString sheet, float scaleW, float scaleH);
    QString ScaleSheetPt(QString sheet, float scaleW, float scaleH);
    QString xPng(QString png);

    void setRegisterMetaType();

    void reLogin(int relogin = 0);

    void proExit(int retcode = 0, bool autoRun = false);      // 0-正常退出  1-更新重启   2-设置自启(管理员权限)

    MainWindow* mainWid();
    void releaseMainWid();

    Login* LoginWid();

    QWidget* CurWid();

    bool toClose();

    QPointer<QTextEdit> m_debugText;

    void exitApp(int code);

    void setMode(QString mode);
    bool isDeadline();

    QStringList fillArgs(QStringList args);

    QPointer<UpHandler> m_upHand;
    QPointer<DownHandler> m_downHand;

signals:
    void sessionExpired(int code);

private:
    QPointer<MainWindow> m_mainWid;
    QPointer<Login> m_loginWid;

    QString m_mode;

    QMutex m_loginMu;
    bool m_loginHandled;
};

#define SESSION Session::instance()

#endif // SESSION_H
