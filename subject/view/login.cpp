#include "login.h"
#include "ui_login.h"

#include "common/trayicon.h"

#include <QDesktopServices>
#include <QIntValidator>
#include <QTextBrowser>
#include <QClipboard>

#include "common/protocol.h"
#include "common/session.h"
#include "tool/network.h"
#include "common/protocol.h"
#include "tool/xfunc.h"

#include "db/userdao.h"
#include "tool/jsonutil.h"
#include "tool/base64.h"
#include "common/buttongroup.h"
#include "tool/msgtool.h"
#include <QCollator>
#include "common/eventfilter.h"
#include "view/set/set.h"
#include "version.h"

#include <QComboBox>
#include <QUuid>
#include <Windows.h>
#include <QDirIterator>

Login::Login(BaseWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::Login),
    bg(NULL),
    m_parent(true),
    mousePress(false)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose,true);

    setResizeable(true);
    setResizeableAreaWidth(10);

    ui->comboBox->setStyleSheet("border-radius:2px;");
    ui->phoneArea->hide();
    ui->loginName->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    ui->loginPassword->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

    setWindowTitle(qApp->applicationDisplayName());

    setMinBtn(ui->headMin);
    setClass(ui->loginBtn, "buttonColor");
    setClass(ui->wxBack, "buttonColor");
    setClass(ui->forgetBtn, "textColor");
    setClass(ui->registerBtn, "textColor");

    ui->loginName->setAlignment(Qt::AlignVCenter);
    ui->loginPassword->setAlignment(Qt::AlignVCenter);

    connect(ui->loginPassword, SIGNAL(returnPressed()), this, SLOT(pressedlogin()));//回车
    connect(ui->loginName, SIGNAL(textChanged(const QString &)), this, SLOT(textChanged(const QString &)));
    connect(ui->loginPassword, SIGNAL(textChanged(const QString &)), this, SLOT(textChanged(const QString &)));
    connect(ui->loginName, &BaseLineEdit::focusOutNoText, this, &Login::focusOutNoText);
    connect(ui->loginName, &BaseLineEdit::focusOut, this, &Login::nameFocusOut);
    connect(ui->loginPassword, &BaseLineEdit::focusOutNoText, this, &Login::focusOutNoText);

    connect(ui->loginPassword, &BaseLineEdit::createTop, this, [=](){
        ui->loginPassword->setToplabel(lineEditlabel, 5, true);
    });
    connect(ui->loginPassword, &BaseLineEdit::deleteTop, this, [=](){
        ui->loginPassword->setToplabel(lineEditlabel, 5, false);
    }, Qt::UniqueConnection);

    connect(ui->loginName, &BaseLineEdit::createTop, this, [=](){
        ui->loginName->setToplabel(lineEditlabel, bg->clickedId(), 1);
    });

    connect(ui->loginName, &BaseLineEdit::deleteTop, this, [=](){
        ui->loginName->setToplabel(lineEditlabel, bg->clickedId(), 0);
    }, Qt::UniqueConnection);

    ui->loginName->setRightIcon(lineEditUser,"user");
    ui->pwdErr->bindTextInteraction();

    QWidget::setTabOrder(ui->loginName, ui->loginPassword);

    QList<QPushButton *> btnL;
    btnL << ui->user << ui->phone << ui->email;
    foreach (QPushButton *btn, btnL) {
        setClass(btn, "switchLogin");
    }
    QList<QFrame *> lineL;
    lineL << ui->userLine << ui->phoneLine << ui->emailLine;
    foreach (QFrame *line, lineL) {
        setClass(line, "switchLine");
    }

    QJsonObject loginInfo = JsonUtil::jsonStrToObj(UserDAO::instance()->readUser(m_parent).info);
    QStringList placeL;
    placeL << tr("用户名") << tr("手机号") << tr("邮箱");
    bg = new ButtonGroup(this);
    connect(bg, &ButtonGroup::btnIdClick, this, [=](int id){
        if (UserInfo::instance()->isOS()) {
            areaCode();
        }

        foreach (QFrame *line, lineL) {
            setProperty(line, "tab", "");
        }
        setProperty(lineL.at(id), "tab", "1");

        ui->loginName->setPlaceholderText(placeL.at(id));

        QJsonObject user = loginInfo.value(QString("loginType%1").arg(id)).toObject();
        qDebug()<< "切换登录类型" << id << user;
        if(id == 0){
            ui->loginName->setRightIcon(lineEditUser, "user");
        }
        if(id == 1){
            ui->loginName->setRightIcon(lineEditUser, "phone");
        }
        if(id == 2){
            ui->loginName->setRightIcon(lineEditUser, "email");
        }

        ui->loginName->setToplabel(lineEditlabel, id, true);

        QString password = Base64::decode(user.value("pwd").toString());
        ui->loginPassword->setText(password);

        ui->loginName->setText(user.value("name").toString());
        ui->comboBox->setCurrentIndex(user.value("phoneCode").toInt());
        ui->remPassC->setChecked(user.value("savepwd").toInt());
        ui->autoLoginC->setChecked(user.value("autologin").toInt());

        ui->loginName->setFocusOutNoText(false);
        connect(ui->loginName, &BaseLineEdit::PasswordFocusIn, [=](){
            qDebug()<<"focusIn"<<id;
            ui->loginName->setinit(false);
            ui->loginName->setToplabel(lineEditlabel, id, true);
        });
        connect(ui->loginName, &BaseLineEdit::PasswordFocusOut, [=](){
            qDebug()<<"focusOUT";
            ui->loginName->setinit(true);
            ui->loginName->setToplabel(lineEditlabel, id, false);
        });
        QTimer::singleShot(1, this, [=](){
            ui->loginName->setFocus();
            if(ui->loginPassword->text().isEmpty()) {//密码栏为空时，如果焦点在密码栏则蓝框、蓝字、下垂，焦点不在则无框、无字、居中,
                    ui->loginPassword->setToplabel(lineEditlabel, 5, false);//删除密码栏的Qlabel
                    ui->loginPassword->setFocusOutNoText(false);
                    setProperty(ui->loginPassword, "clear", "true");
                    setProperty(ui->loginPassword, "text", """");
                    ui->loginPassword->setinit(false);
                    connect(ui->loginPassword, &BaseLineEdit::PasswordFocusIn, [=](){
                        ui->loginPassword->setinit(false);
                        setProperty(ui->loginPassword, "clear", "false");
                    });
                    connect(ui->loginPassword, &BaseLineEdit::PasswordFocusOut, [=](){
                        ui->loginPassword->setinit(true);
                        setProperty(ui->loginPassword, "clear", "true");
                    });
                    connect(ui->loginName, &BaseLineEdit::PasswordFocusOut, [=](){
                        ui->loginName->setinit(true);
                        setProperty(ui->loginName, "clear", "true");
                    });
                    connect(ui->loginName, &BaseLineEdit::PasswordFocusIn, [=](){
                        ui->loginName->setinit(false);
                        setProperty(ui->loginName, "clear", "false");
                    });
            } else {
                ui->loginPassword->setToplabel(lineEditlabel, 5, true);
                ui->loginPassword->setPassword();
                ui->loginPassword->setinit(false);
                setProperty(ui->loginPassword, "clear", "false");
            }
            if (ui->loginName->text().isEmpty()) {
                if (!ui->loginName->hasFocus()) {
                    ui->loginName->setToplabel(lineEditlabel, bg->clickedId(), false);//删除用户名栏的Qlabel
                    ui->loginName->setFocusOutNoText(false);
                    setProperty(ui->loginName, "clear", "true");
                    connect(ui->loginName, &BaseLineEdit::PasswordFocusIn, [=](){
                        ui->loginName->setinit(false);
                        setProperty(ui->loginName, "clear", "false");
                    });
                    connect(ui->loginName, &BaseLineEdit::PasswordFocusOut, [=](){
                        ui->loginName->setinit(true);
                        setProperty(ui->loginName, "clear", "true");
                    });
                }
            } else if (!ui->loginName->hasFocus()){
                ui->loginName->setToplabel(lineEditlabel, bg->clickedId(), true);
                ui->loginName->setPassword();
                ui->loginName->setinit(false);
                setProperty(ui->loginName, "clear", "false");
            }
        });

    });
    bg->addButtons(btnL, qApp->style(), NULL);
    int buttonId = loginInfo.value("loginType").toInt();
    QAbstractButton* button = bg->button(buttonId);
    emit bg->buttonClicked(button);

    ui->loginPassword->setPwdButton();

    setClass(ui->headMin, "min");
    setClass(ui->headClose, "close");

    // foreach (QWidget *wid, QList<QWidget *>()<< ui->headMin << ui->headClose << ui->user << ui->phone << ui->email
    //          << ui->remPassC << ui->autoLoginC << ui->forgetBtn << ui->registerBtn << ui->wx) {
    //     wid->setFocusPolicy(Qt::NoFocus);
    // }

    setProperty(this, "shadowBackground", "loginShadowBackground");
    setProperty(this, "shadowRadius", 32);

    assumeParent();

    setBlurEffectWidget(ui->bgWid);
    setMinimumSize(1056, 752);
    addShadow(ui->loginBtn);
    addShadow(ui->wxBack);

    //保存客户端版本至配置文件
    saveClientVersionToSettings();

    //获取机器配置信息存在延迟，因此使用多线程操作
    workThread = new QThread;
    machineProfiles = new MachineProfiles;
    machineProfiles->moveToThread(workThread);
    connect(workThread, &QThread::started, machineProfiles, &MachineProfiles::getMachineInformation);
    connect(workThread, &QThread::finished, machineProfiles, &MachineProfiles::deleteLater);
    connect(machineProfiles, &MachineProfiles::destroyed, workThread, &QThread::deleteLater);

    manager = new QNetworkAccessManager;
    connect(manager, &QNetworkAccessManager::finished, this, &Login::loginBuryPointReply);
    ipUrl = "https://whois.pconline.com.cn/ipJson.jsp";
    startLoginBuryPoint(ipUrl);

    delPreviousFiles();
}

QString Login::getFileVersion(const QString& filePath) {
    DWORD handle;
    DWORD size = GetFileVersionInfoSizeA(filePath.toStdString().c_str(), &handle);
    if (size == 0){
        return QString();
    }else{
        std::vector<BYTE> versionInfo(size);
        if (!GetFileVersionInfoA(filePath.toStdString().c_str(), handle, size, versionInfo.data())){
            return QString();
        }else{
            VS_FIXEDFILEINFO* fileInfo;
            UINT fileInfoSize;
            if (VerQueryValue(versionInfo.data(), L"\\", (LPVOID*)&fileInfo, &fileInfoSize)) {
                return QString::number(HIWORD(fileInfo->dwFileVersionMS)) + "." +
                       QString::number(LOWORD(fileInfo->dwFileVersionMS)) + "." +
                       QString::number(HIWORD(fileInfo->dwFileVersionLS));
            }else{
                return QString();
            }
        }
    }
    return QString();
}

bool Login::isExistQt6File(const QString &dir){
    QDirIterator it(dir, QStringList() << "*Qt6*", QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();

        if (fileInfo.exists()) {
            qDebug() << "Found Qt6 files:" << fileInfo.absoluteFilePath();
            return true;
        }else{
            qDebug()<<"Not found Qt6 files!";
        }
    }

    return false;
}

void Login::delPreviousFiles(){
    QString appDir = QCoreApplication::applicationDirPath();
    bool isUpdateQt6 = USERINFO->readAllIni("UpdateQt6", QString("isUpdate")).toBool();
    bool isExistFiles = isExistQt6File(appDir);
    if(isExistFiles && !isUpdateQt6){
        QString skipDir = QString("mobao");
        QString compareVersion = QString("5.14.2");
        QStringList filters;
        filters<<QString("*.dll")<<QString("QtWebEngineProcess.exe");
        QDirIterator it(appDir, filters, QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            it.next();
            QFileInfo fileInfo = it.fileInfo();
            QString filePath = fileInfo.absoluteFilePath();

            if (filePath.contains(skipDir, Qt::CaseInsensitive)) {
                continue;
            }else if (fileInfo.fileName().startsWith("Qt5", Qt::CaseInsensitive)){
                qDebug()<<"Remove Qt5 DLL name is:"<<fileInfo.fileName();
                QFile::remove(filePath);
            }else if (fileInfo.fileName() == QString("libssl-1_1.dll") || fileInfo.fileName() == QString("libcrypto-1_1.dll")){
                qDebug()<<"Remove libssl-1_1.dll and libcrypto-1_1.dll";
                QFile::remove(filePath);
            }else if(fileInfo.fileName().contains("api-ms-win")){
                continue;
            }else if(fileInfo.fileName().contains("QtWebEngineProcess.exe")){
                QString version = getFileVersion(filePath);
                if (version == compareVersion) {
                    qDebug()<<"Remove QtWebEngineProcess.exe version 5.14.2 is:"<<filePath;
                    QFile::remove(filePath);
                }else{}
            }else{
                QString version = getFileVersion(filePath);
                if (version == compareVersion) {
                    qDebug()<<"Remove DLL version 5.14.2 is:"<<filePath;
                    QFile::remove(filePath);
                }else{}
            }
        }

        USERINFO->saveAllIni(QString("UpdateQt6"), QString("isUpdate"), true);
    }else{
        qDebug()<<"Version is not corresponding!";
    }
}

Login::~Login()
{
    qDebug() << __FUNCTION__;
    if(manager){
        manager->deleteLater();
    }
    delete ui;
}

void Login::startLoginBuryPoint(QString url){
    QEventLoop loop;
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    QNetworkRequest request(url);
    manager->get(request); // 发起GET请求
    loop.exec();
}

void Login::loginBuryPointReply(QNetworkReply *reply){
    if (reply->error() == QNetworkReply::NoError){
        QByteArray response = reply->readAll();
        QString responseStr = QString::fromLocal8Bit(response);

        QRegularExpression re("IPCallBack\\((.*)\\);");
        QRegularExpressionMatch match = re.match(responseStr);
        if(match.hasMatch()){
            QString jsonStr = match.captured(1);
            QJsonDocument jsonResponse = QJsonDocument::fromJson(jsonStr.toUtf8());
            if(!jsonResponse.isNull() && jsonResponse.isObject()){
                QJsonObject jsonObject = jsonResponse.object();
                ip = jsonObject["ip"].toString();
                ipcityname = jsonObject["city"].toString();
            }else{
                qDebug()<<"Failed to create QJsonDocument or JSON is not an object";
            }
        }
        else{
            qDebug()<<"No JSON data found in response";
        }
    }else{
        qDebug()<<"Error:"<<reply->errorString();
    }
    reply->deleteLater();

    getBuryPointUrl(0); //直接传递参数值，防止影响自动登录状态的变量状态
}

void Login::getBuryPointUrl(int loginStatus){
    //检测环境
    QString buryUrl;
    if(UserInfo::instance()->profiles() == QString("prod")){
        buryUrl = QString("https://www.xrender.com/_.gif");
    }else{
        buryUrl = QString("http://dev-xmax.cudatec.com/_.gif");
    }
    buryUrl = QString("https://www.xrender.com/_.gif");

    //设置vid值
    QString vid;
    if(!USERINFO->readAllIni("UUID", "vid").toString().isEmpty()){
        vid = USERINFO->readAllIni("UUID", "vid").toString();
    }else{
        vid = getUuid(QString("vid"));
        USERINFO->saveAllIni("UUID", "vid", vid);
    }

    QString sessionId;
    QString sessionIdStamp;
    int pvid = 1;
    getSessionIdAndPvidForDifferentEvent(loginStatus, sessionId, sessionIdStamp, pvid); //针对登录事件+登录点击事件生成不同的pvid和sessionId值

    QString pcVersion = getComputerVersion();
    QJsonObject obj;
    QString userId;
    QString eventid;
    if(loginStatus == 1){
        obj.insert("Response", "success");
        userId = UserInfo::instance()->userIdL();
        eventid = QString("0001");
    }else if(loginStatus == 2){
        obj.insert("Response", "fail");
        eventid = QString("0001");
    }else{
        qDebug()<<"eventid is:"<<eventid<<" "<<"obj is:"<<obj;
    }

    QString dynamicmap;
    if(!obj.isEmpty()){
        dynamicmap = QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }else{
        qDebug()<<"dynamicmap is defaule value!";
    }
    QString clickTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString logtime;
    if(loginStatus == 0){
        logtime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    }else{}
    qDebug()<<"dynamicmap and clicktime and logtime is:"<<dynamicmap<<clickTime<<logtime;

    QString fullUrl = QString("%1?"
                              "vid=%2&"
                              "sessionid=%3&"
                              "pvid=1&"
                              "duration=&referer=&prepageid=5000&utm_source=&utm_medium=&utm_campaign=&"
                              "userid=%4&"
                              "pageid=5000&pageurl=&channelid=4&"
                              "operationsystem=%5&"
                              "ip=%6&"
                              "ipcityname=%7&"
                              "dynamicmap=%8&").arg(buryUrl).arg(vid).arg(sessionId).arg(userId).arg(QString(QUrl::toPercentEncoding(pcVersion))).arg(ip).arg(QString(QUrl::toPercentEncoding(ipcityname))).arg(QString(QUrl::toPercentEncoding(dynamicmap)));
    fullUrl.append(QString("logtime=%1&clicktime=%2&").arg(QString(QUrl::toPercentEncoding(logtime))).arg(QString(QUrl::toPercentEncoding(clickTime))));
    fullUrl.append(QString("eventid=%1&activitiesId=&productType=3&dataIndex=1").arg(eventid));
    NET->get(fullUrl, [=](FuncBody f){
            qDebug()<<"fullUrl is:"<<fullUrl<<f.httpCode;
        }, this);
}

void Login::getSessionIdAndPvidForDifferentEvent(int loginStatus, QString &sessionId, QString &sessionIdStamp, int &pvid){
    QString sessionIdKey;
    QString sessionIdStampKey;
    QString pvidKey;
    if(loginStatus == 0){
        //仅登录事件
        sessionIdKey = QString("sessionId");
        sessionIdStampKey = QString("sessionIdStamp");
        pvidKey = QString("pvid");
    }else{
        //仅登录点击事件
        QString click("Click");
        sessionIdKey = QString("sessionId%1").arg(click);
        sessionIdStampKey = QString("sessionIdStamp%1").arg(click);
        pvidKey = QString("pvid%1").arg(click);
    }

    if(!USERINFO->readAllIni("UUID", sessionIdKey).toString().isEmpty()){
        sessionId = USERINFO->readAllIni("UUID", sessionIdKey).toString();
        sessionIdStamp = USERINFO->readAllIni("UUID", sessionIdStampKey).toString();
        pvid = USERINFO->readAllIni("UUID", pvidKey).toInt();

        QDateTime lastTimeStamp = QDateTime::fromString(sessionIdStamp, Qt::ISODate);
        QDateTime currentTimetamp = QDateTime::currentDateTime();
        const qint64 thirtyMinutesInSeconds = 30 * 60;
        if(lastTimeStamp.secsTo(currentTimetamp) > thirtyMinutesInSeconds){
            sessionId = getUuid(QString("sessionId"));
            sessionIdStamp = currentTimetamp.toString(Qt::ISODate);
            pvid = 1;
            USERINFO->saveAllIni("UUID", sessionIdKey, sessionId);
            USERINFO->saveAllIni("UUID", sessionIdStampKey, sessionIdStamp);
            USERINFO->saveAllIni("UUID", pvidKey, pvid);
        }else{
            QString tempSessionId = getUuid(QString("sessionId"));  //判断当sessionId和本地缓存sessionId是否一样
            if(tempSessionId == sessionId){
                pvid++;
                USERINFO->saveAllIni("UUID", pvidKey, pvid);
            }else{
                pvid = 1;
                USERINFO->saveAllIni("UUID", pvidKey, pvid);
            }
        }
    }else{
        pvid = 1;
        sessionId = getUuid(QString("sessionId"));
        sessionIdStamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        USERINFO->saveAllIni("UUID", sessionIdKey, sessionId);
        USERINFO->saveAllIni("UUID", sessionIdStampKey, sessionIdStamp);
        USERINFO->saveAllIni("UUID", pvidKey, pvid);
    }
}

QString Login::getComputerVersion(){
    QString osVersion = QSysInfo::productVersion();
    QString osName = QSysInfo::productType();
    if(!osName.isEmpty()){
        osName.replace(0, 1, osName.at(0).toUpper());
    }else{
    }

    QString pcVersion = QString("%1 %2").arg(osName).arg(osVersion);
    qDebug()<<"pcVersion is:"<<pcVersion;
    return pcVersion;
}

QString Login::getUuid(QString type) {
    QUuid uuid = QUuid::createUuid();
    QString uid = uuid.toString(QUuid::WithoutBraces);
    if(type == "vid"){
        uid.remove('-');
    }else{
    }
    qDebug()<<"uid is:"<<uid;
    return uid;
}


//保存客户端版本至配置文件
void Login::saveClientVersionToSettings(){
    clientVersion = QString("%1").arg(CLIENT_VERSION);
    qDebug()<<"打印版本号："<<clientVersion;
    USERINFO->saveAllIni("officialClientVersion", "version", clientVersion);
}

void Login::areaCode()
{
    if (bg->clickedId() == 1) {//如果是手机号栏就加一个下拉框，里面放手机区号
        qDebug()<<"进入手机栏";
        if (ui->comboBox->currentText().isEmpty()) {
            QString path = ":/phone area code.txt";
            QFile file(path);
            if (!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
                qDebug()<<"文件打开出错";
                ui->phoneArea->show();
                ui->comboBox->setCurrentText("error");
                ui->loginName->setFixedWidth(214);
            }
            QTextStream in(&file);
            QStringList list;
            while (!in.atEnd()) {
                QString line = in.readLine(); // 读取一行
                list.append(line); // 添加到 text edit
            }
            qDebug()<<list;
            file.close();
            ui->comboBox->addItems(list);
        }
        ui->phoneArea->show();
        ui->loginName->setFixedWidth(214);
    } else {//如果不是手机号栏就删除上述下拉框
        qDebug()<<"不在手机栏";
        ui->phoneArea->hide();
        ui->loginName->setFixedWidth(326);
    }
}

void Login::addShadow(QWidget *w)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setOffset(0, 7);//水平位移，垂直位移
    QPalette pal = w->palette();
    QColor color = pal.color(QPalette::Window);
    effect->setColor(QColor(19, 125, 211, 102));
    effect->setBlurRadius(29);//阴影扩散半径
    w->setGraphicsEffect(effect);
}

void Login::userPress()
{

}

void Login::phonePress()
{

}

void Login::emailPress()
{

}
void Login::initLogin(int relogin, QJsonObject user)
{
    qDebug()<<"开始";
    showNameErr("", false);
    showPwdErr("", false);
    hide();
    TrayIcon::instance()->setCurWid(this);

    bool autoLogin = false;
    if (m_ssoCode.isEmpty()) {
        bool parent = !UserDAO::instance()->readAllUser("loginChild").toBool();
        if (user.isEmpty()) {
            db_user u = UserDAO::instance()->readUser(parent);
            qDebug() << __FUNCTION__ << u.info << u.autologin;
            QJsonObject loginInfo = JsonUtil::jsonStrToObj(u.info);
            user = loginInfo.value(QString("loginType%1").arg(loginInfo.value("loginType").toInt())).toObject();
        }
        if (0 != relogin || !Set::loginSilent()) {
            show();
            ui->loginName->setFocus();
        }
    }

    // 更新完成后自动登录
    if (relogin == UpdateAutoLogin && !user.isEmpty()) {
        autoLogin = true;
    } else {
        if (0 == relogin && 1 == user.value("autologin").toInt()) {
            autoLogin = true;
        } else if (relogin == 10403 || relogin == 13403 || relogin == 14403 || relogin == 15403) {
            showPwdErr(codeView(QString::number(relogin)), true);
        } else if (relogin == 12403 ){
            showRemoteLoginErr();
        }

    }

    if (autoLogin || !m_ssoCode.isEmpty()) {
        QTimer::singleShot(1, this, [=](){
            btnEnable(false);
            qDebug()<<"自动登录";
            on_loginBtn_clicked();
        });
    }else{
        btnEnable(true);    //登录按钮置为可点击，需要在判断是否自动登录之后，否则会存在及时性问题
    }

}

void Login::setSsoCode(QString code)
{
    m_ssoCode = code;
}

bool Login::isSsoLogin()
{
    return !m_ssoCode.isEmpty();
}

QJsonObject Login::machineInfo()
{
    MachineInfo info;
    XFunc::SYS(info);
    QString mac = XFunc::MAC();
    QString vol = XFunc::VOL();
    QString sw = QString::number(XFunc::ScreenWidth());
    QString sh = QString::number(XFunc::ScreenHeight());

    QJsonObject obj;
    obj.insert("os", info.OS);
    obj.insert("cpu", info.CPU);
    obj.insert("mac", mac);
    obj.insert("disk", vol);
    obj.insert("mainboard", info.MotherBoard);
    obj.insert("graphics", info.DisplayCard);
    obj.insert("memory", info.Memory);
    obj.insert("screenW", sw);
    obj.insert("screenH", sh);
    obj.insert("maxVersions", "");
    return obj;
}

bool Login::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->phoneArea) {
        switch (event->type()) {
        case QEvent::FocusIn:
            setProperty(ui->label_2, "type", "0");
            break;
        case QEvent::FocusOut:
            setProperty(ui->label_2, "type", "1");
            break;
        default:
            break;
        }
    }
    return  QWidget::eventFilter(watched, event);
}

void Login::mousePressEvent(QMouseEvent *event){
    if((event->button() == Qt::LeftButton)){
        mousePress = true;
        mousePoint = event->globalPos() - this->pos();
    }else{
    }
}

void Login::mouseReleaseEvent(QMouseEvent *event){
    mousePress = false;
}

void Login::mouseMoveEvent(QMouseEvent *event){
    if(mousePress){
        move(event->globalPos() - mousePoint);
    }else{}
    this->update();
}

void Login::assumeParent()
{
    changeAppname(true);
}

void Login::btnEnable(bool enable)
{
    ui->loginName->setEnabled(enable);
    ui->loginPassword->setEnabled(enable);
    ui->remPassC->setEnabled(enable);
    ui->autoLoginC->setEnabled(enable);
    ui->forgetBtn->setEnabled(enable);
    ui->loginBtn->setEnabled(enable);//登录按钮样式自动回弹
    ui->registerBtn->setEnabled(enable);
    if (enable)
        show();
}

void Login::on_headClose_clicked()
{
    Session::instance()->proExit(0);
}

void Login::on_forgetBtn_clicked()
{
    QDesktopServices::openUrl(QUrl(USERINFO->openForgetPwd()));
}

bool Login::IsValidPhoneNumber(const QString &phoneNum)
{
    if (!UserInfo::instance()->isOS()) {
        QRegularExpression regx("^[0-9]{11}$");
        QRegularExpressionValidator regs(regx, 0);
        QString pNum = phoneNum;
        int pos = 0;
        QValidator::State res = regs.validate(pNum, pos);
        if (QValidator::Acceptable == res) {
            return true;
        }
        else {
            return false;
        }
    } else {
        QRegularExpression regx("^[0-9]{3,20}$");
        QRegularExpressionValidator regs(regx, 0);
        QString pNum = phoneNum;
        int pos = 0;
        QValidator::State res = regs.validate(pNum, pos);
        if (QValidator::Acceptable == res) {
            return true;
        }
        else {
            return false;
        }
    }
}

bool Login::IsValidEmail(const QString &email)
{
    QRegularExpression regx("^[A-Za-z0-9\u4e00-\u9fa5]+@[a-zA-Z0-9_-]+(\.[a-zA-Z0-9_-]+)+$");
    QRegularExpressionValidator regs(regx, 0);
    QString pEmail = email;
    int pos = 0;
    QValidator::State res = regs.validate(pEmail, pos);
    if (QValidator::Acceptable == res) {
        return true;
    }
    else {
        return false;
    }
}

int Login::authType()
{
    int t = 0;
    switch (bg->clickedId()) {
    case 0:
        t = 3;
        break;
    case 1:
        t = 1;
        break;
    default:
        t = 2;
        break;
    }
    return t;
}

void Login::showNameErr(QString err, bool show)
{
    if (show) {
        ui->nameTip->show();
        ui->nameErr->show();
        ui->nameErr->setText(err);
        this->show();
    } else {
        ui->nameTip->hide();
        ui->nameErr->hide();
    }

}

void Login::showPwdErr(QString err, bool show)
{
    if (show) {
        if (err.isEmpty())
            return;
        ui->pwdTip->show();
        ui->pwdErr->show();
        ui->pwdErr->setText(err);
        qDebug()<<err;
        this->show();
    } else {
        ui->pwdTip->hide();
        ui->pwdErr->hide();
    }
}

void Login::showMacErr()
{
    MsgBox *mb = MsgTool::msgChoose("", this);
    mb->setTitle(tr("请注意"));
    mb->setTitleIcon(MsgBox::Warn);
    mb->setOkText(tr("前往个人中心"));
    mb->setNoText(tr("我知道了"));
    mb->setOkClose(false);
    mb->setFixedHeight(234);

    QWidget *contentWid = new QWidget(mb);
    mb->setContentWidget(contentWid);

    QVBoxLayout *lay = new QVBoxLayout(contentWid);
    lay->setContentsMargins(QMargins());

    QTextBrowser *tb = new QTextBrowser(contentWid);
    tb->setText(tr("您当前无法登录客户端，如需授权登录请前往个人中心进行机器码授权配置。"));
    lay->addWidget(tb);

    {
        QWidget *copyWid = new QWidget(contentWid);
        QHBoxLayout *copyLay = new QHBoxLayout(copyWid);
        copyLay->setContentsMargins(QMargins());

        QLabel *mac = new QLabel(copyWid);
        BaseWidget::setClass(mac, "loginMac");
        QString text = QString(tr("此电脑机器识别码为 ") + XFunc::MAC(true));
        mac->setText(text);
        copyLay->addWidget(mac);

        QPushButton *copy = new QPushButton(copyWid);
        copy->setFixedSize(12, 12);
        BaseWidget::setClass(copy, "lineEditCopyBtn");
        copyLay->addWidget(copy);
        connect(copy, &QPushButton::clicked, this, [=]{
            qApp->clipboard()->setText(XFunc::MAC(true));
        });

        copyLay->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        copyWid->setLayout(copyLay);
        lay->addWidget(copyWid);
    }

    connect(mb, &MsgBox::accepted, this, []{
        QDesktopServices::openUrl(QUrl(USERINFO->openMacSet()));
    });
}

void Login::showRemoteLoginErr()
{
    MsgBox *mb = MsgTool::msgChoose("" , this);
    mb->setTitle(tr("请注意"));
    mb->setOkText(tr("修改密码"));
    mb->setNoText(tr("我知道了"));


    qgetenv("date");
    QString dateString = qgetenv("date");
    QString formatString = "ddd, d MMM yyyy hh:mm:ss 'GMT'";
    QDateTime date = QLocale(QLocale::English).toDateTime(dateString,formatString);
    date.setTimeSpec(Qt::UTC);
    date = date.toLocalTime();

    connect(mb, &MsgBox::accepted, this, [=](){
        QDesktopServices::openUrl(QUrl(USERINFO->openForgetPwd()));
    });

    mb->loginChange();
    QString lan = Set::changeLan();
    if(dateString==""){
        if(lan == "en_us"){
            QString text1 = QString(tr("当前账户在非常用设备上登录，"));
            mb->setText1(text1);
            QString text2 = QString(tr("若非本人操作，"));
            mb->setText2(text2);
            QString pass = QString(tr("请立即修改账号"));
            mb->setPasswordChange(pass);
            QString text3 = QString(tr("为了"));
            mb->setText3(text3);
            QString text4 = QString(tr("您的账号安全"));
            mb->setText4(text4);

        }else{
            QString text1 = QString(tr("当前账户在非常用设备上登录，若非本人操作，为了您的"));
            mb->setText1(text1);
            QString text2 = QString(tr("账号安全请立即"));
            mb->setText2(text2);
        }

    } else {
        if(lan == "en_us"){
            QString text1 = QString(tr("当前账户于%1在非常用上登录，").arg(date.toString("hh:mm")));
            mb->setText1(text1);
            QString text2 = QString(tr("设备.若非本人操作，"));
            mb->setText2(text2);
            QString pass = QString(tr("请立即修改账号"));
            mb->setPasswordChange(pass);
            QString text4 = QString(tr("为了您的账号安全"));
            mb->setText4(text4);
            mb->hideText3();
        }else{
            QString text1 = QString(tr("当前账户于%1在非常用设备上登录，若非本人操作，为了").arg(date.toString("hh:mm")));
            mb->setText1(text1);
            QString text2 = QString(tr("您的账号安全请立即"));
            mb->setText2(text2);
        }

    }
}

void Login::onSingleConnected()
{
    if (QLocalServer *server = static_cast<QLocalServer *>(sender())) {
        QLocalSocket *socket = server->nextPendingConnection();
        socket->waitForReadyRead(1000);
        QByteArray b = socket->readAll();
        qDebug()<< __FUNCTION__ << socket << b;
        if (b == "exit") {
            qApp->exit(0);
            return;
        }
        QByteArray resp;
        socket->write(resp);
        socket->flush();
    }
}

void Login::on_loginBtn_clicked()
{
    int isLoginSuccess = 2; //初始化点击登录时为2，只有登录成功才会切换为1
    int loginType = bg->clickedId();
    QString name = ui->loginName->text();

    if (!nameFocusOut())
        return;

    if (ui->loginPassword->text().isEmpty()) {
        ui->loginPassword->setFocusOutNoText(true);
        return;
    }

    showNameErr("", false);
    showPwdErr("", false);
    btnEnable(false);

    int auType = authType();
    QJsonObject query;
    query.insert("mac", XFunc::MAC());
    query.insert("password", XFunc::Md5(ui->loginPassword->text()));
    QString url;

    //母账号登录
    if (m_parent) {
        QString centerCode = m_ssoCode;
        if (centerCode.isEmpty()) {
            query.insert("authorizeType", auType);
            query.insert("authorizeKey", name.trimmed());
            query.insert("clientId", USERINFO->clientId());
            if (UserInfo::instance()->isOS() && bg->clickedId() == 1) {
                qDebug()<<"加入区号"<< ui->comboBox->currentText();
                query.insert("areaCode", ui->comboBox->currentText());
            }

            QPointer<Login> ptr = this;
            FuncBody f = NET->sync(POST, accountCenterUrl, query);
            if (!ptr)
                return;
            QString code = f.j.value("data").toObject().value("data").toObject().value("code").toString();
            if (code.isEmpty()) {
                getBuryPointUrl(isLoginSuccess);
                int c = codeI(f.b);
                QString m = codeView(f.b, CodeSection::LoginCenter);
                switch (c) {
                case 100:
                case 101:
                case 102:
                    nameError();
                    ui->loginName->setTextError(true);
                    break;
                case 104:
                case 16:
                    showNameErr(m, true);
                    ui->loginName->setTextError(true);
                    break;
                case 103:
                    passwordError();
                default:
                    showPwdErr(m, true);
                    qDebug()<<m;
                    ui->loginPassword->setTextError(true);
                    break;
                }
                btnEnable(true);
                return;
            }
            centerCode = code;
        }

        query = QJsonObject();
        query.insert("clientId", USERINFO->clientId());
        query.insert("code", centerCode);
        url = loginAuthUrl;
    } else { //子账号登录
    }

    NET->post(url + XFunc::toQuery(query), JsonUtil::jsonObjToByte(query), [=](FuncBody f){
        if (f.succ) {
            QJsonObject obj = f.j;
            int code = obj.value("code").toInt();
            if (code == 200) {
                QJsonObject data = obj.value("data").toObject();
                int resultCode = data.value("code").toInt();
                if (resultCode == 1) {
                    data = data.value("data").toObject();
                    QString bsToken = data.value("token").toString();
                    QString userName = data.value("nickname").toString();
                    qDebug()<< "获取bs token" << bsToken;
                    USERINFO->setAccessToken(bsToken.toUtf8());
                    QString userId = data.value("id").toString();
                    qDebug()<< "获取新userId" << userId;
                    USERINFO->setUserIdL(userId);
                    USERINFO->setUserName(userName);
                    NET->xrpost("/bs/user/login", JsonUtil::jsonObjToByte(UserInfo::instance()->machineInformation), [=](FuncBody f) {
                        int status = f.j.value("status").toInt();
                        if (11030000 == status || USERINFO->isOS()) {
                            int userId = f.j.value("userId").toInt();
                            QString userName = f.j.value("username").toString();
                            if (userId != 0)
                                USERINFO->setUserId(QString::number(userId));

                            db_user u;
                            QJsonObject user, userInfo;
                            QJsonObject info = JsonUtil::jsonStrToObj(UserDAO::instance()->readUser(m_parent).info);
                            user.insert("name", ui->loginName->text());
                            if (ui->remPassC->isChecked()) {
                                user.insert("pwd", Base64::encode(ui->loginPassword->text().toUtf8()));
                                user.insert("savepwd", 1);
                            } else {
                                user.insert("pwd", "");
                                user.insert("savepwd", 0);
                            }
                            if (ui->autoLoginC->isChecked()) {
                                user.insert("autologin", 1);
                            } else {
                                user.insert("autologin", 0);
                            }
                            Set::setAutoLogin(ui->autoLoginC->isChecked());
                            if(loginType == 1){
                               user.insert("phoneCode",ui->comboBox->currentIndex());
                            }

                            info.insert("loginType", loginType);
                            info.insert(QString("loginType%1").arg(loginType), user);
                            u.info = JsonUtil::jsonObjToStr(info);
                            qDebug() << __FUNCTION__ << u.info;
                            if (m_ssoCode.isEmpty())
                                UserDAO::instance()->saveUser(u, m_parent);
                            UserDAO::instance()->setAllUser("loginChild", !m_parent);

                            userInfo = user;
                            userInfo.insert("pwd", Base64::encode(ui->loginPassword->text().toUtf8()));
                            userInfo.insert("permission", f.j.value("permission").toObject());
                            USERINFO->initUser(USERINFO->userName(), userInfo);
                            USERINFO->setParent(m_parent);

                            int loginSuccess = 1;
                            getBuryPointUrl(loginSuccess);
                            qDebug()<<"登录界面已隐藏";
                            this->close();      //主界面出现后，登录界面关闭并结合属性销毁该对象
                            Session::instance()->mainWid()->initMainWid();
                        } else {
                            getBuryPointUrl(isLoginSuccess);
                            showPwdErr(f.j.value("detail").toString(), true);
                            btnEnable(true);
                        }
                    }, this);
                } else {
                    getBuryPointUrl(isLoginSuccess);
                    showPwdErr(codeView(f.b, CodeSection::LoginCenter), true);
                    btnEnable(true);
                }
            } else {
                getBuryPointUrl(isLoginSuccess);
                showPwdErr(obj.value("message").toString(), true);
                btnEnable(true);
            }
        } else {
            getBuryPointUrl(isLoginSuccess);
            showPwdErr(tr("网络错误"), true);
            btnEnable(true);
        }
    }, this);
}

void Login::on_registerBtn_clicked()
{
    QDesktopServices::openUrl(USERINFO->instance()->openRegister());
}

void Login::pressedlogin()
{
    on_loginBtn_clicked();
}

void Login::textChanged(const QString &text)
{
    BaseLineEdit *le = qobject_cast<BaseLineEdit *>(sender());
    if (ui->loginName == le) {
        showNameErr("", false);
    } else if (ui->loginPassword == le) {
        showPwdErr("", false);
    }
}

void Login::on_autoLoginC_clicked(bool checked)
{
    qDebug() << __FUNCTION__ << checked;
    if (checked)
        ui->remPassC->setChecked(true);
}

void Login::on_remPassC_clicked(bool checked)
{
    qDebug() << __FUNCTION__ << checked;
    if (!checked)
        ui->autoLoginC->setChecked(false);
}

void Login::on_headMin_clicked()
{
    this->showMinimized();
}

void Login::on_wx_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget->clearFocus();
    changeEnShow();
    setFocus();
}

void Login::on_wxBack_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Login::on_en_link_clicked(){
  QDesktopServices::openUrl(QUrl(USERINFO->openDiscord()));
}

void Login::focusOutNoText(bool noText)
{
    BaseLineEdit *le = qobject_cast<BaseLineEdit *>(sender());
    if (!le)
        return;

    if (isMinimized() || qApp->applicationState() != Qt::ApplicationActive)
        noText = false;

    QString text = noText ? le->placeholderText() : "";
    QString tip = QString(tr("请输入") + text);
    if (ui->loginName == le){
        showNameErr(tip, noText);
    }
    else{
        showPwdErr(tip, noText);
    }
}

bool Login::nameFocusOut()
{
    int loginType = bg->clickedId();
    QString name = ui->loginName->text();
    if (name.isEmpty()) {
        ui->loginName->setFocusOutNoText(true);
        return false;
    } else {
        QString error;
        if (1 == loginType) {
            if (!IsValidPhoneNumber(name.trimmed()))
                error = tr("手机号填写错误，请重新填写");
        }
        if (2 == loginType) {
            if (!IsValidEmail(name.trimmed()))
                error = tr("邮箱填写错误，请重新填写");
        }
        if (!error.isEmpty()) {
            ui->loginName->setTextError(true);
            showNameErr(error, true);
            return false;
        }
    }

    return true;
}

void Login::nameError()
{
    int loginType = bg->clickedId();
    QString name = ui->loginName->text();
    if (name.isEmpty()) {
        ui->loginName->setFocusOutNoText(true);
    } else {
        QString error;
        if (0 == loginType) {
                error = tr("用户名填写错误，请重新填写");
        }
        if (1 == loginType) {
            if (IsValidPhoneNumber(name.trimmed()))
                error = tr("手机号填写错误，请重新填写");
        }
        if (2 == loginType) {
            if (IsValidEmail(name.trimmed()))
                error = tr("邮箱填写错误，请重新填写");
        }
        if (!error.isEmpty()) {
            ui->loginName->setTextError(true);
            showNameErr(error, true);
        }
    }
}

void Login::passwordError()
{
    ui->loginPassword->setTextError(true);
}

void Login::changeEnShow(){
     QString lan = Set::changeLan();
     if(lan == "en_us"){
         ui->wxCodeText->hide();
         ui->en_box->show();
     }else{
         ui->wxCodeText->show();
         ui->en_box->hide();
     }
}
