#include "widget.h"
#include "ui_widget.h"

#include <QProcess>
#include <QDebug>
#include <QTextCodec>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegExp>
#include <QTextStream>
#include <QDir>
#include <QRegularExpressionValidator>
#include <QGraphicsDropShadowEffect>
#include <QStyle>

#include "../xfunc.h"

#include <QDirIterator>
#include <QLibrary>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <Windows.h>

Widget::Widget(BaseWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::Widget),
    m_clear(false)
{
    ui->setupUi(this);

    ui->opinionText->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);


    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(36, 36);//水平位移，垂直位移
    effect->setColor(QColor(0, 0, 0, 64));
    effect->setBlurRadius(47);//阴影扩散半径
    setGraphicsEffect(effect);

        layout()->setContentsMargins(64, 64, 64, 64);

    setClass(ui->minBtn, "min");
    setClass(ui->closeBtn, "close");
    setClass(ui->finish, "buttonColor");
    setClass(ui->next, "buttonColor");


    ui->frameWid->hide();
    ui->ingWid->hide();
    ui->afterWid->hide();

    addShadow(ui->next);
    addShadow(ui->finish);

    setFixedSize(656+128, 424+128);

    ui->opinionText->setPlaceholderText(tr("请输入您对CG magic的其他意见..."));

    m_net = new QNetworkAccessManager(this);

   if (BaseWidget::langType()) {
       connect(m_net, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

       QString us = "http://bs.xrender.com/cms/uninstallReason/getPresentByType?type=2";
       m_net->get(QNetworkRequest(QUrl(us)));
   } else {
       hideChecked();
   }


    if (BaseWidget::langType()) {
        ui->afterText->setFixedSize(252, 22);
    } else {
        ui->next->setFixedSize(210, 32);
        ui->afterText->setFixedSize(421, 22);
    }

    readCilentExeVersion();
    getSqlFile();
    getEnvironmentJsonPath();
}
//读取客户端exe的版本号（非读取配置文件）
void Widget::readCilentExeVersion(){
    QString clientExePath = m_instPath + QString("/") + m_exe;
    qDebug()<<"clientExePath is:"<<clientExePath;

    //检查该路径下的客户端程序是否存在
    if(!QFile::exists(clientExePath)){
        qDebug()<<"cgmagic.exe is not exists!";
    }else{
        QString versionNumber;
        DWORD dummy;
        DWORD versionSize = GetFileVersionInfoSize(clientExePath.toStdWString().c_str(), &dummy);
        if (versionSize == 0) {
            qDebug()<<"Error getting file version info size";
            return;
        }

        std::vector<BYTE> versionData(versionSize);
        if (!GetFileVersionInfo(clientExePath.toStdWString().c_str(), 0, versionSize, versionData.data())) {
            qDebug()<<"Error getting file version info";
            return;
        }

        VS_FIXEDFILEINFO *fileInfo;
        UINT len;
        if (VerQueryValue(versionData.data(), L"\\", (LPVOID *)&fileInfo, &len)) {
            if (fileInfo->dwSignature == 0xfeef04bd) {
                versionNumber = QString::number(HIWORD(fileInfo->dwFileVersionMS)) + "." +
                                QString::number(LOWORD(fileInfo->dwFileVersionMS)) + "." +
                                QString::number(HIWORD(fileInfo->dwFileVersionLS)) + "." +
                                QString::number(LOWORD(fileInfo->dwFileVersionLS));
            }
        }
        clientVersion = versionNumber;
        qDebug()<<"cgmagic exe version is:"<<clientVersion;
    }
}

//获取客户端的版本号
void Widget::getClientVersion(){
    QString userIniPath = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
    QSettings settings(userIniPath, QSettings::NativeFormat);
    QString iniPath = settings.value("AppData").toString();
    iniPath.replace('\\', '/');
    iniPath.append("/Xcgmagic/AllUser/config.ini");
    qDebug()<<"iniPath路径"<<iniPath;

    QSettings setIni(iniPath, QSettings::IniFormat);
    QString versionCode = setIni.value(QString("%1/%2").arg("officialClientVersion").arg("version")).toString();

    clientVersion = versionCode;
    qDebug()<<"当前客户端的版本号："<<clientVersion;
}

//获取对应的配置文件内容
void Widget::getSqlFile(){
    QString userSqlPath = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
    QSettings settings(userSqlPath, QSettings::NativeFormat);
    QString sqlPath = settings.value("AppData").toString();
    sqlPath.replace('\\', '/');
    sqlPath.append("/Xcgmagic/AllUser/user.db");
    qDebug()<<"数据库文件："<<sqlPath;

    QSqlDatabase userDb = QSqlDatabase::addDatabase("QSQLITE");
    userDb.setDatabaseName(sqlPath);
    if (!userDb.open()) {
        qDebug() << "Error: Unable to open database";
        return;
    }

    // 查询数据
    QSqlQuery query("SELECT * FROM user");
    while (query.next()) {
        qDebug() << "query.value(0):" << query.value(0);
        qDebug() << "query.value(1):" << query.value(1);

        QJsonDocument jsonDoc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject loginType0 = jsonObj.value("loginType0").toObject();
        QString nameValue = loginType0.value("name").toString();
        userName = nameValue;
    }
    userDb.close();
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

Widget::~Widget()
{
    delete ui;
}

void Widget::closeEvent(QCloseEvent *e)
{
    if (m_clear && !m_instPath.isEmpty() && m_instPath.contains("CG MAGIC")) {
        QDir dir(m_instPath);
        QFile file(QDir::toNativeSeparators(QDir::tempPath()) + "\\uninstall.bat");
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream in(&file);
            in << "ping -n 5 127.0.0.1>nul\r\n";   //通用版的卸载时间设定为5s，旧版本是1s导致卸载不干净
            QByteArray s("rd \"");
            s.append(dir.absolutePath().replace("/","\\").toUtf8()).append("\" /s /q");
            in << s <<"\r\ndel %0";
            in.flush();
            file.close();

            QProcess clear;
            clear.startDetached(file.fileName());
        }
    }

    e->accept();
}

void Widget::uninstall()
{
    //点击下一步删除的时候，需要发送数据
    QJsonObject obj;
    obj.insert("reason", deleteReason);
    obj.insert("clientVersion", clientVersion);
    obj.insert("clientType", 1);
    obj.insert("additionalComments", otherTextFromDelete);
    obj.insert("nickname", userName);
    qDebug()<<"具体的发送内容："<<obj;

    QJsonDocument document;
    document.setObject(obj);
    QNetworkRequest request;

    request.setRawHeader("appname", "web");
    request.setRawHeader("Content-Type", "application/json");

    QString us = url + "/cgmagic-console/uninstall/add";
    qDebug()<<"实际的请求地址us："<<us;
    request.setUrl(QUrl(us));
    QNetworkReply *reply =  m_net->post(request, document.toJson(QJsonDocument::Compact));
    connect(reply,  &QNetworkReply::finished, [=](){
        QByteArray data = reply->readAll();
        QString str = QString(data);
        qDebug()<<"发送数据给后端！"<<document.toJson(QJsonDocument::Compact);
        qDebug()<<"响应数据data"<<str;
    });

    ui->frameWid->show();
    ui->beforWid->hide();
    ui->ingWid->show();
    ui->logo->hide();

    pro(10);

    {
        typedef void (*KillExeByDirP)(char*, char*);
        QLibrary lib("killExe.dll");
        if (lib.load()) {
            if (KillExeByDirP kill = (KillExeByDirP)lib.resolve("KillExeByDir"))
                kill(m_instPath.toLocal8Bit().data(), QString("uninstall.exe").toUtf8().data());
        }
    }

    pro(20);

    {
        QProcess pro;
        pro.start(QDir(m_instPath).filePath(m_exe), QStringList()<< "-dms");
        pro.waitForFinished();
    }

    pro(40);

    if (!m_name.isEmpty())
        veryDel(stdAppPath() + "/" + m_name);
    if (!m_name.isEmpty())
        veryDel(stdDeskPath() + "/" + QString("%1.lnk").arg(m_name));

    if (!m_nameEN.isEmpty())
        veryDel(stdAppPath() + "/" + m_nameEN);
    if (!m_nameEN.isEmpty())
        veryDel(stdDeskPath() + "/" + QString("%1.lnk").arg(m_nameEN));

    pro(60);

    if (!m_instPath.isEmpty())
        qDebug()<< "remove install path" << QDir(m_instPath).removeRecursively() << m_instPath;

    pro(80);

    m_uninstSet->remove(thirdGroup());
    if (!m_exe.isEmpty())
        m_autoRunSet->remove(m_exe);

    removeClientSet();

    postReason();
    finished();
}

//获取对应环境的cg地址
void Widget::getEnvironmentJsonPath(){
    QString userJsonPath = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
    QSettings settings(userJsonPath, QSettings::NativeFormat);
    QString jsonPath = settings.value("AppData").toString();
    jsonPath.replace('\\', '/');
    jsonPath.append("/Xcgmagic/AllUser/service.json");
    qDebug()<<"jsonPath路径"<<jsonPath;

    getUrlFromJson(jsonPath);
}

//从Json获取指定地段的网址
void Widget::getUrlFromJson(QString jsonPath){
    QFile file(jsonPath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug()<<"Failed to open JSON file";
        return;
    }
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug()<<"Error parsing JSON:"<<parseError.errorString();
        return;
    }

    QJsonObject jsonObject = jsonDoc.object();
    QString fieldValue;
    if (jsonObject.contains("cgmagic"))
    {
        fieldValue = jsonObject.value("cgmagic").toString();
        qDebug()<<"cgmagic:"<< fieldValue;
    }

    url = fieldValue;
    qDebug()<<"请求地址url:"<<url;
    file.close();   //关闭文件

    //新增删除json配置文件操作，以防止国内外配置文件冲突造成影响
    if(url.isEmpty()){
        qDebug()<<"Url is empty!";
    }else{
        if(QFile::remove(jsonPath)){
            qDebug()<<"Json file remove successfully!"<<jsonPath;
        }else{
            qDebug()<<"Json file remove failure!"<<file.error();
        }
    }
}

void Widget::pro(int p)
{
    ui->ingText_2->setText(tr("正在卸载") + QString("(%1%)").arg(p));
    ui->ingText_2->adjustSize();
    ui->progressBar->setValue(p);
    if (ui->progressBar->value() == ui->progressBar->maximum())
        ui->progressBar->hide();
}


QString Widget::name()
{
    return m_cn ? m_name : m_nameEN;
}

void Widget::on_minBtn_clicked()
{
    this->showMinimized();
}

void Widget::on_closeBtn_clicked()
{
    this->close();
}

void Widget::on_cancel_clicked()
{
    this->close();
}


void Widget::on_next_clicked()
{
    uninstall();
}

void Widget::finished()
{
    //完成
    pro(100);

    ui->closeBtn->show();
    m_clear = true;

    ui->frameWid->hide();
    ui->ingWid->hide();
    ui->afterWid->show();
    ui->logo->show();
    if(isSlientUninstall){
        setAppExit();
        return;
    }
}

//设置静默卸载的状态，true表示静默卸载，false表示非静默卸载
void Widget::setSlientUninstallStatus(bool slientUninstall){
    isSlientUninstall = slientUninstall;
}

void Widget::setAppExit(){
    QTimer::singleShot(2000, this, [=](){
        on_finish_clicked();
    });
}

void Widget::on_finish_clicked()
{
    close();
    qApp->exit(0);
}

void Widget::on_opinionText_textChanged()
{
    QString textContent = ui->opinionText->toPlainText();
    int length = textContent.count();
    int maxLength = 1000; // 最大字符数
    if(length > maxLength) {
        int position = ui->opinionText->textCursor().position();
        QTextCursor textCursor = ui->opinionText->textCursor();
        textContent.remove(position - (length - maxLength), length - maxLength);//删除起点，删除长度
        ui->opinionText->setText(textContent);
        textCursor.setPosition(position - (length - maxLength));
        ui->opinionText->setTextCursor(textCursor);
    }//对超出部分删除并调整光标位置

    length = textContent.count();
    ui->opinionLabel->setText(QString("%1/%2").arg(length).arg(1000));

    if(length==1000){
        setProperty(ui->opinionLabel, "overflow", "over");
        setProperty(ui->opinionText, "overflow", "over");
    }else {
        setProperty(ui->opinionLabel, "overflow", "noover");
        setProperty(ui->opinionText, "overflow", "noover");
    }
    otherTextFromDelete = ui->opinionText->toPlainText();
}

void Widget::postReason()
{
    QList<QCheckBox *> checkL = ui->beforWid->findChildren<QCheckBox *>();
    QJsonObject obj;
    QJsonArray arr;

    for (int i = 0; i < checkL.length(); ++i) {
        QCheckBox *check = checkL.at(i);
        if (!check->text().isEmpty() && check->isChecked()) {
            int id = check->toolTip().toInt();
            QJsonObject o;
            o.insert("id", id);
            arr.append(o);
        }
    }

    QString other = ui->opinionText->toPlainText();
    if (!other.isEmpty()) {
        QJsonObject o;
        o.insert("id", 0);
        o.insert("other", ui->opinionText->toPlainText());
        o.insert("contact", "");
        arr.append(o);
    }

    obj.insert("rows", arr);
    obj.insert("type", 2);
    qDebug()<<obj;

    QJsonDocument document;
    document.setObject(obj);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString us = "http://bs.xrender.com/cms/uninstallReason/getUserUninstallReasonByType";
    request.setUrl(QUrl(us));
}

void Widget::replyFinished(QNetworkReply *reply)
{
    qDebug()<<reply->error()<<reply->errorString();
    if (reply->error() == QNetworkReply::NoError) {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if(status >= 200 && status < 300) {
            QString debug = QString::fromUtf8(reply->readAll());
            qDebug()<<debug;

            QJsonObject obj = QJsonDocument::fromJson(debug.toUtf8()).object();
            QJsonArray arr = obj.value("rows").toArray();
            QMap<int, QJsonObject> order;
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject o = arr.at(i).toObject();
                int idx = o.value("order").toInt();
                order.insert(idx, o);
            }

            QList<QCheckBox *> checkL = ui->beforWid->findChildren<QCheckBox *>();
            QList<QJsonObject> orderL = order.values();
            for (int i = 0; i < qMin(orderL.length(), checkL.length()); ++i) {
                QJsonObject o = orderL.at(i);
                QCheckBox *check = checkL.at(i);

                //检查QCheckBox的状态，通过信号槽
                connect(check, &QCheckBox::stateChanged, this, [=](int state){
                    //根据状态添加卸载原因索引
                    int index = orderL.at(i).value("id").toInt();
                    if(state == 2){  //2表示选中
                        deleteReasonFromIndex.insert(index, orderL.at(i).value("reason").toString());
                    }else{
                        deleteReasonFromIndex.remove(index);
                    }

                    qDebug()<<"deleteReasonFromIndex显示内容："<<deleteReasonFromIndex;

                    if(!deleteReason.isEmpty()){
                        deleteReason.clear();
                    }

                    if(!deleteReasonFromIndex.isEmpty()){
                        QList<QString> values = deleteReasonFromIndex.values();
                        for(auto value: values){
                            deleteReason.append(value);
                            deleteReason.append(" ");
                        }
                    }
                });

                check->setText(o.value("reason").toString());
            }

            foreach (QCheckBox *check, checkL) {
                if (check->text().isEmpty())
                    check->hide();
            }
        }
    }

    disconnect(m_net, 0, 0, 0);
}

void Widget:: hideChecked(){
      QList<QCheckBox *> checkL = ui->beforWid->findChildren<QCheckBox *>();
      foreach (QCheckBox *check, checkL) {
              check->hide();
      }
}
