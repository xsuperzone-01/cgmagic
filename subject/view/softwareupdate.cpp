#include "softwareupdate.h"
#include "ui_softwareupdate.h"
#include <QJsonArray>
#include "common/protocol.h"
#include "tool/network.h"
#include "tool/jsonutil.h"
#include "common/session.h"
#include "versions/versionmanager.h"
#include <QtConcurrent>
#include <QDirIterator>
#include "common/eventfilter.h"
#include "common/trayicon.h"

SoftwareUpdate::SoftwareUpdate(QWidget *parent) :
    ui(new Ui::SoftwareUpdate),
    m_force(false)
{
    ui->setupUi(this);
    modaldel();//模态化
    raise();
    activateWindow();
    if (parent) {
        connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
        QPoint gp = parent->mapToGlobal(QPoint(0,0));
        move(gp.x()+(parent->width()-width())/2, gp.y()+(parent->height()-height())/2);
    }

    TrayIcon::instance()->setCurWid(this);

    m_timer.setInterval(1000);
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeOut()));

    setClass(ui->okBtn, "okBtn");
    setClass(ui->noBtn, "noBtn");
    setClass(ui->headMin, "min");
    setClass(ui->headClose, "close");
    setMinBtn(ui->headMin);

    addShadow(ui->okBtn);

    setFixedSize(656, 424);

    setWindowTitle(qApp->applicationDisplayName());
    show();

    //目前云桌面和呆猫是同一个更新包
    m_updateSoftIds << 25;
    m_admin = false;


    ui->headMin->hide();

}

SoftwareUpdate::~SoftwareUpdate()
{
    if (m_timer.isActive())
        m_timer.stop();

    removeDetail();
    delete ui;
}

void SoftwareUpdate::addShadow(QWidget *w)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 7);//水平位移，垂直位移
    QPalette pal = w->palette();
    QColor color = pal.color(QPalette::Window);
    effect->setColor(QColor(19, 125, 211, 102));
    effect->setBlurRadius(29);//阴影扩散半径
    w->setGraphicsEffect(effect);
}

void SoftwareUpdate::initDetail(const QJsonObject obj, bool autoStart, bool force)
{

    QJsonArray forceV = obj.value("force").toArray();

    if (!forceV.isEmpty()) {
        QJsonObject obj = forceV.first().toObject();
        ui->version->setText(tr("版本号：%1").arg(obj.value("version").toString()));

        QString detail = VersionManager::instance()->splitDetail(obj.value("detail").toString());
        ui->textBrowser->setText(detail);
    }

    m_force = force;
    if (force) {
        ui->noBtn->hide();
    }

    // 需要用户点击
    autoStart = false;
    if (force && !USERINFO->userIdL().isEmpty()) {
        ui->headMin->hide();
        ui->headClose->hide();
    }

    if (!autoStart) {
        ui->progressBar->setValue(0);
        ui->error->clear();
        ui->widget_4->hide();
        ui->widget->show();
    } else {
        on_okBtn_clicked();
    }

    show();
}



bool SoftwareUpdate::admin()
{
    return m_admin;
}

void SoftwareUpdate::removeDetail()
{
}

bool SoftwareUpdate::needMove(QMouseEvent *e)
{
    return false;//禁止移动
}

void SoftwareUpdate::update()
{
#ifdef Q_OS_WIN
    {
        bool admin = false;
        EventLoop loop(0);
        qDebug()<< "start check file access";
        QtConcurrent::run([=, &admin, &loop]{
            QDirIterator d(qApp->applicationDirPath(), QDir::Files, QDirIterator::Subdirectories);
            while (d.hasNext()) {
                d.next();

                QString file = d.filePath();
                HANDLE h = CreateFile((LPCWSTR)file.unicode(), FILE_WRITE_EA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                if (h != INVALID_HANDLE_VALUE)
                    CloseHandle(h);
                else {
                    admin = true;
                    break;
                }
            }
            loop.quit();
        });
        qDebug()<< "end check file access"<<admin;
        if (loop.execUntilAppQuit())
            return;

        if (qgetenv("XDEMO_ADMIN_UPGRADE") != "")
            admin = true;

        qDebug()<<"loop execFinished is:"<<admin;
        if (admin) {
            VersionManager *vm = VersionManager::instance();
            vm->exitProcess();
            m_admin = true;
            int ret = XFunc::runAsAdmin(vm->exe(), vm->args().join(" "));
            if (-1 == ret) {
                qDebug()<< "cancel admin upgrade";
                showUpdateStatus(-2);
                return;
            }
        }
    }
#endif

    force();
}

void SoftwareUpdate::timeOut()
{
    NET->get(updateStatusUrl, [=](FuncBody f){
        int code = f.j.value("data").toObject().value("code").toInt();
        if (code == 200) {
            int value = ui->progressBar->value();
            int status = f.j.value("data").toObject().value("data").toObject().value("status").toInt();
            if (-1 == status) {//修改文案随进度条
                if (value < 90) {
                    QString s = ui->error->text();
                    if (s.endsWith("%)")){
                        s = s.chopped(5);
                    }
                    int v = value + 10;
                    ui->progressBar->setValue(v);
                    QString x = QString(s + QString("(%1%)").arg(v));
                    ui->error->setText(x);
                }
            } else if (status == -2) {
                qDebug() << "update err:" << f.j;
                showUpdateStatus(status);//-2
            } else if (status == 0) {
                ui->progressBar->setValue(100);
                showUpdateStatus(status);//0
                QEventLoop loop;
                QTimer::singleShot(3000, &loop, SLOT(quit()));
                loop.exec();
                close();
                qDebug()<<"fsdjkfufjksdfhjksdferqwerfsd";
            } else if (status == 1094) {    // 自启
                ui->progressBar->setValue(100);
                showUpdateStatus(status);//1094
                QEventLoop loop;
                QTimer::singleShot(3000, &loop, SLOT(quit()));
                loop.exec();
                Session::instance()->proExit(1);
            }
        } else {
            qDebug() << "get update status err:" << f.j;
        }

        m_timer.start();
    }, this);
}

void SoftwareUpdate::showUpdateStatus(int status)
{
    QString s;
    if (status == -2) {
        s = tr("更新失败");
        ui->progressBar->hide();
    } else if (status == 0) {
        s = tr("更新成功");
    } else if (status == 1094) {
        s = tr("更新成功，正在重启...");
    } else if (status == 10)
        s = tr("文件下载中");
    else if (status == 11)
        s = tr("正在更新");
    ui->error->setText(s);
    ui->error->show();

    // 下载中禁止关闭
    if (status == -2)
        ui->headClose->show();
    else
        ui->headClose->hide();
}

void SoftwareUpdate::on_okBtn_clicked()
{
    ui->widget_5->hide();

    ui->itemBg->hide();
    ui->widget->hide();
    ui->widget_4->show();
    ui->progressBar->show();
    ui->progressBar->setValue(0);

    showUpdateStatus(10);

    update();
}

void SoftwareUpdate::force()
{
    QJsonObject obj;
    obj.insert("isForceUpgrade", true);
    NET->put(updateUrl, JsonUtil::jsonObjToByte(obj), [=](FuncBody f) {

        //windows以管理员重启更新程序后，数据还在准备中
        int status = f.j.value("data").toObject().value("data").toObject().value("status").toInt();
        if (f.j.isEmpty() || status == 0) {
            showUpdateStatus(10);
            int b = f.b.replace("文件下载中", "").replace("\"", "").toInt();
            ui->progressBar->setValue(b);
            ui->error->setText(QString(tr("文件下载中(%1%)").arg(b)));
            QTimer::singleShot(1000, this, SLOT(force()));
            return;
        }

        int code = f.j.value("data").toObject().value("code").toInt();
        if (code == 200) {
            showUpdateStatus(11);
            ui->progressBar->setValue(0);
            m_timer.start();
        } else {
            qDebug() << "to force update err:" << f.j;
            showUpdateStatus(-2);
        }
    }, this);
}

//下次提醒
void SoftwareUpdate::on_noBtn_clicked()
{
    emit nextClicked();
    deleteLater();
}

void SoftwareUpdate::on_headMin_clicked()
{
}

void SoftwareUpdate::on_headClose_clicked()
{
    if (m_force) {
        qApp->exit(0);
        return;
    }
    deleteLater();
}
