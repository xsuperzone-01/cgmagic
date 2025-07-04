#include "widget.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDebug>

#include "../msgbox.h"
#include <QObject>
#include "../singleapp.h"
#include <QTranslator>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QProcess>

int main(int argc, char *argv[])
{
    bool isUninstall = false;
    for(int i = 0; i<argc; i++){
        qDebug()<<"argv[i]:"<<argv[i];
        if(QString(argv[i]).contains("--uninstall")){
            isUninstall = true;
            break;
        }
    }

    SingleApp a(argc, argv);

    if(isUninstall){       //修改SingleApp类中的变量服务名称ServerName;
        a.setServerName(true);
    }else{
        a.setServerName(false);
    }

    if (a.exist())
        return 0;

    qDebug()<<"SingleApp是否存在，仅仅只适用一个服务状态";

    //语言
    QTranslator tran;
    BaseWidget b;
    QString lan = b.lang();
    if (lan.isEmpty()) {
        lan = "zh_cn";
    }
    bool ret = tran.load(QString(":/language/%1.qm").arg(lan));
    if (ret) {
        a.installTranslator(&tran);
    }
    qDebug()<<__func__<<lan;


    QString fontFamily= "";
    bool m_cn = b.langType();
    if(m_cn){
       fontFamily = "Microsoft Yahei";
    }else{
       fontFamily = "Arial";
    }
    QFont font;font.setFamily(fontFamily);font.setPixelSize(12);
    qApp->setFont(font);
    qApp->setStyleSheet(BaseWidget::globalStyle(m_cn));

    if (BaseWidget::existProcess("3dsmax.exe")) {
        MsgTool::msgOkLoop(QObject::tr("请先关闭max，再卸载客户端。"));
        return 0;
    }

    if(BaseWidget::existProcess("cgmagic.exe")){
        QString appName = "cgmagic.exe";
        QString command = "taskkill";
        QStringList arguments;
        arguments<<"/IM"<<appName<<"/F"<<"/T";
        QProcess process;
        process.start(command, arguments);  //启动进程

        if (!process.waitForStarted()) {  //检查进程是否成功启动
            qDebug()<<"Fail start process!";
            return -1;
        }else{
            qDebug()<<"Success start process!";
        }

        if(!process.waitForFinished(2000)){
            qDebug()<<"Fail close cgmagic.exe in 2 seconds!";
            return -1;
        }else{
            qDebug()<<"Success close cgmagic.exe!";
        }
    }

    Widget w;
    if(isUninstall){
        qDebug()<<"uninstall为true,则表示静默卸载";
        w.setSlientUninstallStatus(true);   //表示静默卸载，需要将卸载程序退出
        w.hide();
        w.on_next_clicked();
    }else{
        qDebug()<<"uninstall为false,则表示非静默卸载";
        w.show();
    }

    return a.exec();
}
