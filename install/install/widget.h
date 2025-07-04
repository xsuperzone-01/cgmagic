#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPainter>

#include "../basewidget.h"
#include <QPushButton>

namespace Ui {
class Widget;
}

class Widget : public BaseWidget
{
    Q_OBJECT

public:
    explicit Widget(BaseWidget *parent = 0);
    ~Widget();

    void cover(QString newIns);
    QString defaultInstallPath();
    QString mainExe();


signals:
    void installFinish();



private:
    void pro(int p);

    QString name();

    void fillPathSuffix();

    void addShadow(QWidget *w);

    void backMove(QWidget *w);

    void readDoc();

    qint64 getFreeSpaceForUserInstallPath(QString path);  //获取用户选择安装路径的磁盘剩余空间大小

    void changeWidgetsStatus(bool status);  //修改界面上控件状态

    bool startProcess(const QString &program, int timeout);

public:
    void checkDirExistAndDeleteFiles(const QString& folderPath);  //检查程序的安装目录是否存在，若目录下存在文件则删除

private slots:
    void on_install_clicked();

    void on_minBtn_clicked();

    void on_closeBtn_clicked();

    void on_browse_clicked();

    void zipOver();
    void oneOk();

    void clientSetSuccess();
    void copyPublicFileSuccess();

    void on_run_clicked();

    void on_license_clicked();

    void on_licenseBack_clicked();

private:
    Ui::Widget *ui;

    QString m_version;
    bool m_installOk;

    QPushButton *openDir;
    QProcess *uninstallProcess;

    bool isDeleteClientSet = false;   //用于二次安装的时候，进行析构函数是否进行removeClientSet
    bool status = false;
};

#endif // WIDGET_H
