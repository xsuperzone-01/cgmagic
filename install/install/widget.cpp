#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QProcess>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCodec>
#include <QString>
#include <QByteArray>
#include <QDesktopServices>
#include <QDateTime>
#include <QSettings>
#include <QScrollArea>
#include <QPropertyAnimation>

#include "quazip/JlCompress.h"
#include "zipthread.h"
#include "../xfunc.h"

#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include "../msgbox.h"
#include <QElapsedTimer>
#include "installprocess.h"
#include "../threadpool.h"
#include <QGraphicsDropShadowEffect>
#include <QLibrary>
#include <QStorageInfo>
#include <QPainterPath>
#include <QStringList>

Widget::Widget(BaseWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::Widget),
    m_installOk(false)
{
    ui->setupUi(this);

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(36, 36);//水平位移，垂直位移
    effect->setColor(QColor(0, 0, 0, 64));
    effect->setBlurRadius(47);//阴影扩散半径
    setGraphicsEffect(effect);

    layout()->setContentsMargins(64, 64, 64, 64);

    setFixedSize(656+128, 424+128);

    setClass(ui->minBtn, "min");
    setClass(ui->closeBtn, "close");
    setClass(ui->afterText, "afterText");

    setClass(ui->install,"buttonColor");
    setClass(ui->run,"buttonColor");
    setClass(ui->licenseBack,"buttonColor");

    addShadow(ui->install);

    openDir = new QPushButton(this);
    openDir->setFixedSize(16, 16);
    BaseWidget::setClass(openDir, "lineEditOpenDir");
    connect(openDir, &QPushButton::clicked, this, &Widget::on_browse_clicked);
    ui->path->setRightButton(openDir, 12);

    ui->path->setReadOnly(true);
    m_instPath = defaultInstallPath();
    fillPathSuffix();
    qDebug()<< "Widget: install path" << m_instPath;
    ui->path->setText(m_instPath);
    ui->path->setCursorPosition(0);

    ui->autoRun->setChecked(false);
    hide();

    ui->stackedWidget->setCurrentIndex(0);
    ui->ingWid->hide();
    ui->afterWid->hide();
    ui->logoWid->hide();

    if (BaseWidget::langType()) {
        QHBoxLayout *hlay = new QHBoxLayout(this);
        hlay->addWidget(ui->autoRun);
        hlay->addSpacing(36);
        hlay->addWidget(ui->agree);
        ui->bottomWid->setLayout(hlay);
        ui->afterText->setFixedSize(192, 72);
    } else {
        ui->license->setStyleSheet("font-style: oblique;text-decoration:underline;");

        QVBoxLayout *vlay = new QVBoxLayout(this);
        vlay->addWidget(ui->autoRun);
        vlay->addWidget(ui->agree);
        vlay->addSpacing(15);
        ui->bottomWid->setLayout(vlay);
        readDoc();

    }

    uninstallProcess = new QProcess(this);
}

void Widget::readDoc()
{
    QFile sf(":/Xrender Cloud Service Terms.txt");
    sf.open((QFile::ReadOnly));
    QByteArray sb = sf.readAll();
    sf.close();
    ui->textBrowser->setText(sb);
}

void Widget::backMove(QWidget *w)
{
    QLabel *background = new QLabel(w);//新建一个qlabel用于防止背景图
    QPixmap image(":/background.jpg");//加载背景图
    QPixmap scaledImage = image.scaledToHeight(w->height(), Qt::SmoothTransformation);//把图片的高度调整为窗口的高度，同时保持图片的宽高比，并且使用平滑变换来处理图像。


    background->setGeometry(w->rect());//设置为父类窗口的位置和大小
    background->lower();//控件高度沉底，不加这一行的话，图片会挡住底下的按钮，无法点击。
    QPixmap map = scaledImage.copy(0, 0, w->width(), w->height());//将调整后的图片从(0,0)到父类宽高进行拷贝
    background->setPixmap(map);//赋予一张初始图像，以免计时开始之前背景为空。
    background->show();
    // 创建圆角矩形区域
    int radius = 24;//已知圆角半径为24px
    QPainterPath path;
    path.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(w->width(), w->height())), radius, radius);//宽，高,圆角半径。（起点，终点，圆角半径，四个角都一样）。椭圆角矩形，椭圆x=椭圆y时为圆角。
    QRegion region(path.toFillPolygon().toPolygon());//将path创建为一个QRegion对象。
    background->setMask(region);//设置label蒙皮大小为如上尺寸

    QTimer *count = new QTimer;
    int *x = new int;//左起相对位置
    int *y = new int;//右起相对位置
    //声明int数为实例化对象是为了让lambda表达式捕获并进行赋值。
    *x = 0;
    *y = scaledImage.size().width();
    int *s = new int;//画布左起位置
    connect(count, &QTimer::timeout, w, [=](){
        if(*x < scaledImage.size().width() - w->width()){
            *x = *x + 1;
            *s = *x;
            if(*x == (scaledImage.size().width() - w->width())){
                *y = 0;
            }
        }
        if(*y < scaledImage.size().width() - w->width()){
            *y = *y + 1;
            *s = scaledImage.size().width() - w->width() -*y;
            if(*y == (scaledImage.size().width() - w->width())){
                *x = 0;
            }
        }
        QPixmap map = scaledImage.copy(*s, 0, w->width(), w->height());
        background->setPixmap(map);
        background->show();
    });

    connect(w, &Widget::destroyed, [=](){
        delete x;
        delete y;
        delete s;
        count->deleteLater();
    });//窗口关闭时清除上面实例化的对象（指针）。
    count->start(10);

}


Widget::~Widget()
{
    if(!isDeleteClientSet){
        removeClientSet(false);   //旧版安装程序暂时不需要移除注册表设置，看测试结果;
    }
    delete ui;
}

void Widget::addShadow(QWidget *w)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 7);//水平位移，垂直位移
    QPalette pal = w->palette();
    QColor color = pal.color(QPalette::Window);
    effect->setColor(QColor(19, 125, 211, 102));
    effect->setBlurRadius(29);//阴影扩散半径
    w->setGraphicsEffect(effect);
}

void Widget::cover(QString newIns)
{
    if (newIns.isEmpty())
        return;
    isDeleteClientSet = true;
    m_instPath = newIns;
    ui->install->setProperty("ignoreLic", true);

    //优化后覆盖安装静默卸载
    QString uninstallPath= m_instPath + "uninstall.exe";
    qDebug()<<"uninstallPath is:"<<uninstallPath;
    bool ok = startProcess(uninstallPath, 10000);

    qDebug()<<"覆盖安装的路径:"<<m_instPath;
    checkDirExistAndDeleteFiles(m_instPath);
    if(ok){
        qDebug()<<"ok是否为true："<<ok;
        on_install_clicked();
    }else{
        qDebug()<<"ok是否为false："<<ok;
    }
}

//检查程序的安装目录是否存在，若目录下存在文件则删除
void Widget::checkDirExistAndDeleteFiles(const QString &folderPath){
    qDebug()<<"确认该文件目录路径:"<<folderPath;

    if(folderPath.isEmpty()){
        qDebug()<<"文件路径为空;";
        return;
    }

    QDir folder(folderPath);
    if (!folder.exists()) {
        qDebug()<<"该文件目录不存在："<<folderPath;
        return;
    }

    QStringList valueList = folderPath.split("\\");
    qDebug()<<"valueList:"<<valueList;
    QString valueForCG = "CG MAGIC";
    if(valueList.contains(valueForCG)){
        qDebug()<<"该路径存在CG MAGIC目录";
    }else{
        return;
    }

    if (folder.removeRecursively()) {
        qDebug()<<"该目录下内容删除成功"<<folderPath;
    } else {
        qDebug()<<"该目录下内容删除失败"<<folderPath;
    }
}

bool Widget::startProcess(const QString &program, int timeout){
    QEventLoop loop;
    QTimer timer;

    connect(uninstallProcess, &QProcess::started, this, [=, &loop](){
        int state = uninstallProcess->state();
        if(state == QProcess::Running){
            qDebug()<<"Process state is normal!";
        }else{
            qDebug()<<"Process start unnormal and state is:"<<uninstallProcess->errorString();
            status = false;
            loop.quit();
        }
    });

    connect(uninstallProcess, &QProcess::errorOccurred, this, [=, &loop](QProcess::ProcessError error){
        qDebug()<<"ProcessError is:"<<error<<uninstallProcess->errorString();
        status = false;
        loop.quit();
    });

    connect(uninstallProcess, &QProcess::finished, this, [=, &loop](int exitCode, QProcess::ExitStatus exitStatus){
        if(exitStatus == QProcess::NormalExit){
            qDebug()<<"Process finished successfully with exit code:"<<exitCode;
            status = true;
        }else{
            qDebug()<<"Process was terminated and exitStatus is:"<<exitStatus;
            status = false;
        }
        loop.quit();
    });

    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [=, &loop](){
        qDebug()<<"Process timed out!";
        status = false;
        loop.quit();
    });

    QStringList arguments;
    arguments<<"--uninstall";
    uninstallProcess->start(program, arguments);
    timer.start(timeout);
    loop.exec();

    return status;
}

QString Widget::defaultInstallPath()
{
    return QProcessEnvironment::systemEnvironment().value("PROGRAMFILES") + "\\" + "XRender";
}

QString Widget::mainExe()
{
    return QDir::toNativeSeparators(QDir(m_instPath).filePath(m_exe));
}


void Widget::pro(int p)
{
    ui->ingText_2->setText(tr("正在安装") + QString("(%1%)").arg(p));
    if (p < ui->progressBar->value())
        return;

    ui->progressBar->setValue(p);
}

QString Widget::name()
{
    return m_cn ? m_name : m_nameEN;
}

void Widget::fillPathSuffix()
{
    qDebug()<<"m_instPathFont："<<m_instPath;
    QStringList installList = m_instPath.split(":");
    if(installList.last() == "/"){    //用于判断是否在磁盘根目录安装
        QString firstString = installList.first();
        installList.removeLast();
        installList.removeFirst();

        m_instPath = firstString + ":" + installList.join("");
        qDebug()<<"m_instPathMiddle"<<m_instPath;
    }
    m_instPath = QDir::toNativeSeparators(m_instPath);
    qDebug()<<"m_instPathLast"<<m_instPath;

    QString sfx = "CG MAGIC";
    QString sfx2 = sfx + "\\";
    if (!m_instPath.endsWith(sfx) && !m_instPath.endsWith(sfx2)) {
        m_instPath += "\\" + sfx;
    }
    qDebug()<<"m_instPathFinal"<<m_instPath;
}

qint64 Widget::getFreeSpaceForUserInstallPath(QString userInstallPath){
    QStorageInfo storageInfo;
    storageInfo.setPath(userInstallPath);
    storageInfo.refresh();
    if (storageInfo.isValid() && storageInfo.isReady()) {
        qDebug() << "rootpath:"<< storageInfo.rootPath();
        qDebug()<<"available bytes:"<<storageInfo.bytesAvailable();
        return storageInfo.bytesAvailable();
    }
    return -1; // 表示获取失败
}

void Widget::changeWidgetsStatus(bool status){
    ui->licenseCheck->setEnabled(status);
    ui->autoRun->setEnabled(status);
    ui->minBtn->setEnabled(status);
    ui->closeBtn->setEnabled(status);
    ui->install->setEnabled(status);
    openDir->setEnabled(status);
}

void Widget::on_install_clicked()
{
    QString diskName = m_instPath.split(":").at(0);
    qDebug()<<"diskName:"<<diskName<<QString("%1%2").arg(diskName).arg(tr(":"));
    ui->path->setText(m_instPath);
    qint64 freeSpace = getFreeSpaceForUserInstallPath(QString("%1%2").arg(diskName).arg(tr(":")));
    if(freeSpace/(1024*1024) <= 1024){   //至少存在2GB的安装空间才行
        changeWidgetsStatus(false);
        int ret = MsgTool::msgChooseLoop(QObject::tr("磁盘安装空间不足，请重新选择磁盘！"));  //0表示确定，1表示取消

        if(ret == 0){
            changeWidgetsStatus(true);
            on_browse_clicked();
        }
        changeWidgetsStatus(true);
        return;
    }

    if (!ui->install->property("ignoreLic").toBool() && !ui->licenseCheck->isChecked()) {
        ui->licenseBack->setProperty("install", true);
        on_license_clicked();
        return;
    }

    ui->beforWid->hide();
    ui->logoWid->hide();
    ui->ingWid->show();
    ui->frameWid->layout()->setContentsMargins(QMargins());
    setFixedSize(656+128, 424+128);

    QFileInfo f(m_appPath + "/files.zip");
    qDebug()<<"exist zip"<<f.exists();

    fillPathSuffix();
    QDir().mkpath(m_instPath);

    qDebug()<<"sdfhjkhsdjkfh is:"<<m_instPath<<m_appPath;

    // 火绒会检测killExe.exe为病毒
    typedef void (*KillExeByDirP)(char*, char*);
    QLibrary lib("killExe.dll");
    if (lib.load()) {
        if (KillExeByDirP kill = (KillExeByDirP)lib.resolve("KillExeByDir"))
            kill(m_instPath.toLocal8Bit().data(), QString("").toUtf8().data());
    }

    {
        InstallProcess* tw = new InstallProcess();
        THP->initThreadPool(tw);
        connect(tw, &InstallProcess::copyPublicFileSuccess, this, &Widget::copyPublicFileSuccess);
        QINVOKE(tw, "copyPublicFile", Q_ARG(QString, m_appPath),
                Q_ARG(QString, m_instPath));
    }
}

void Widget::on_minBtn_clicked()
{
    this->showMinimized();
}

void Widget::on_closeBtn_clicked()
{
    this->close();
}

void Widget::on_browse_clicked()
{
    if(this->isHidden()){
        this->show();
        ui->stackedWidget->setCurrentIndex(0);
        ui->ingWid->hide();
        ui->afterWid->hide();
        ui->beforWid->show();
        ui->frameWid->show();
    }

    QString t = (m_cn ? "选择安装目录" : "Choose install dir");
    QString path = QFileDialog::getExistingDirectory(this, t, "/home", QFileDialog::ShowDirsOnly);
    qDebug()<<"getExistingDirectory:"<<path;
    if (!path.isEmpty()) {
        m_instPath = path;
        fillPathSuffix();
        ui->path->setText(m_instPath);
        ui->path->repaint();
        ui->path->setCursorPosition(0);
    }
}

void Widget::zipOver()
{
    pro(80);

    InstallProcess* ct = new InstallProcess();
    QJsonObject obj;
    obj.insert("instPath", m_instPath);
    obj.insert("appPath", m_appPath);
    obj.insert("exe", m_exe);
    obj.insert("name", m_name);
    obj.insert("nameEN", m_nameEN);
    obj.insert("version", m_version);
    obj.insert("third", m_third);
    obj.insert("lang", m_lang);
    obj.insert("mark", m_mark);
    obj.insert("namedef", name());
    obj.insert("startMenu", true);
    obj.insert("autoRun", ui->autoRun->isChecked());
    obj.insert("cn", m_cn);

    THP->initThreadPool(ct);
    connect(ct, &InstallProcess::pro, this, [=]{
        pro(qMin(ui->progressBar->value() + 4, 100));
    });
    connect(ct, SIGNAL(installClientSetSuccess()), this, SLOT(clientSetSuccess()));
    QINVOKE(ct, "clientSet", Q_ARG(QJsonObject, obj));
}

void Widget::oneOk()
{
    static int count = 0;
    count++;
    pro(qMin(count, 80));
}

void Widget::clientSetSuccess()
{
    m_installOk = true;
    pro(100);
    ui->frameWid->hide();
    ui->ingWid->hide();
    ui->afterWid->show();
    ui->logoWid->show();
    addShadow(ui->run);

    emit installFinish();
}

void Widget::copyPublicFileSuccess()
{
    //-解压同目录下的files.zip到安装目录
    ZipThread* th = new ZipThread;
    connect(th, SIGNAL(finished()), this, SLOT(zipOver()));
    connect(th, SIGNAL(oneOk()), this, SLOT(oneOk()));

    th->setFileDir(m_appPath + "/files.zip", m_instPath);
    th->start();
}



void Widget::on_run_clicked()
{
    QProcess::startDetached("explorer", QStringList()<< mainExe());
    this->close();
}

// textBrowser文本来源 https://www.xrender.com/v23/service
void Widget::on_license_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget->clearFocus();
    setFocus();
    ui->logoWid->show();
    ui->licenseBack->setEnabled(false);
    bool install = ui->licenseBack->property("install").toBool();

    QString text = tr("我已知晓");
    if (install) {
        text = tr("我已知晓并安装");
    }
    QString timeText = text + "(%1s)";
    ui->licenseBack->setText(timeText.arg(3));

    QTimer *timer = new QTimer(this);
    timer->setProperty("count", 3);
    timer->start(1000);
    connect(timer, &QTimer::timeout, this, [=]{
        int c = timer->property("count").toInt() - 1;
        timer->setProperty("count", c);
        ui->licenseBack->setText(timeText.arg(c));

        if (c <= 0) {
            addShadow(ui->licenseBack);
            ui->licenseBack->setText(text);
            ui->licenseBack->setEnabled(true);
            timer->deleteLater();
        }
    });
}

void Widget::on_licenseBack_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->licenseCheck->setChecked(true);


    if (ui->licenseBack->property("install").toBool()) {
        on_install_clicked();
    }
}
