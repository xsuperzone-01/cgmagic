#include "widget.h"
#include <QApplication>

#include "../msgbox.h"
#include "../singleapp.h"
#include <QObject>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QAbstractSocket>
#include <QPointer>
#include "../xfunc.h"
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTranslator>

int main(int argc, char *argv[])
{
    SingleApp a(argc, argv);

    QCommandLineOption silentOp("silent");
    QCommandLineOption pathOp("path");
    pathOp.setValueName("path");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addOption(silentOp);
    parser.addOption(pathOp);
    parser.process(a);

    bool silent = parser.isSet(silentOp);
    qDebug()<< "isSilent:" << silent;

    if (a.exist())
        return 0;

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

    Widget w;

    QSettings* cset = w.clientSet();
    if (!cset)
        return 0;

    QString ins = cset->value("InstallDir").toString();
    qDebug()<< "已经安装路径" << ins;

    if (BaseWidget::existProcess("3dsmax.exe")) {
        MsgTool::msgOkLoop(QObject::tr("请先关闭max，再安装客户端。"));
        return 0;
    }

    if (!silent) {
        if (!ins.isEmpty()) {
            int ret = MsgTool::msgChooseLoop(QObject::tr("客户端已安装，是否覆盖安装？"));
            if (ret != 0)
                return 0;
        }
    }


    if (!ins.isEmpty()) {
    } else {
        if (silent)
            ins = w.defaultInstallPath();
    }

    if (parser.isSet(pathOp))
        ins = QString::fromStdWString(parser.value(pathOp).toStdWString());
    w.cover(ins);
    if (!silent) {
        w.show();
    } else {
        QObject::connect(&w, SIGNAL(installFinish()), &w, SLOT(on_run_clicked()));
    }

    ins.clear();

    return a.exec();
}
