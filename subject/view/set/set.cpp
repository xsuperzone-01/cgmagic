#include "set.h"
#include "ui_set.h"

#include <QtConcurrent>
#include "common/basewidget.h"
#include "tool/xfunc.h"
#include "common/session.h"
#include "config/userinfo.h"
#include <QApplication>
#include <QClipboard>

#define SetG "Set"

bool Set::m_autoLogin;
QString Set::m_cacheDir;

Set::Set(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Set)
{
    ui->setupUi(this);

    ui->widget_5->show();
    ui->path->hide();
    ui->lang->setMaxVisibleItems(5);
    ui->cacheDir->setMaxVisibleItems(5);

    foreach (QLabel *l, QList<QLabel *>()<< ui->updateTip << ui->loginHideTip << ui->loginSilentTip << ui->cacheTip) {
        BaseWidget::setClass(l, "i");
    }
    ui->loginHideTip->hide();
    ui->loginSilentTip->hide();
    ui->updateTip->hide();
    ui->cacheTip->hide();

    BaseWidget::setClass(ui->openCacheDir, "noBtn");

    connect(ui->copyBtn, &QPushButton::clicked, this, [=]{
        qApp->clipboard()->setText(ui->mac->text());
    });

    QMap<int, QString> langMap;
    QString lan = Set::changeLan();
    if(lan == "en_us"){
       langMap.insert(UserInfo::language_en, "English");
    }else{
        langMap.insert(UserInfo::language_cn, "中文");
    }

    QList<int> kl = langMap.keys();
    QStringList vl = langMap.values();
    for (int i = 0; i < kl.length(); ++i) {
        ui->lang->addItem(vl.at(i), kl.at(i));
    }
    ui->lang->setCurrentText(langMap.value(Langnum()));
    //语言修改
    connect(ui->lang, static_cast<void (ComboBox::*)(int)>(&ComboBox::currentIndexChanged), this, &Set::setLang, Qt::UniqueConnection);

    connect(ui->update, &QCheckBox::clicked, this, [=](bool checked){
        setUpdateStableVersion(checked);
    }, Qt::UniqueConnection);

    connect(ui->loginSilent, &QCheckBox::clicked, this, [=](bool checked){
        setLoginSilent(checked);
        if (checked) {
            ui->loginHide->setChecked(true);
            emit ui->loginHide->clicked(true);
        }
        ui->loginHide->setEnabled(!checked);
    }, Qt::UniqueConnection);

    QStringList diskL = XFunc::getDistNames();
    foreach (QString disk, diskL) {
        ui->cacheDir->addItem(disk + "\\CGMAGIC");
    }
    connect(ui->cacheDir, static_cast<void (ComboBox::*)(int)>(&ComboBox::currentIndexChanged), this, &Set::setCache, Qt::UniqueConnection);
    QPushButton *openDir = new QPushButton(this);
    openDir->setFixedSize(16, 16);
    BaseWidget::setClass(openDir, "lineEditOpenDir");
    connect(openDir, &QPushButton::clicked, this, &Set::on_openCacheDir_clicked);
    ui->path->setRightButton(openDir, 12);

    ui->path->setReadOnly(true);
    ui->path->setCursorPosition(0);


    connect(ui->note, &QPushButton::clicked, this, [=](){
        if(ui->widget_8->isVisible()){
            ui->widget_8->hide();
             ui->label_2->setWordWrap(false);
            BaseWidget::setProperty(ui->note, "type", "down");
        }else {
            ui->widget_8->show();
             ui->label_2->setWordWrap(true);
            BaseWidget::setProperty(ui->note, "type", "up");
        }
    });

    ui->label_2->setWordWrap(true);
    if(lan == "en_us"){
       QString txt2 = "<div style='line-height:18px;'>" + ui->label_2->text() + "</div>";
       ui->label_2->setText(txt2);
    }
}

Set::~Set()
{
    delete ui;
}

void Set::initSet()
{
    ui->autoRun->setChecked(USERINFO->isAutoRun());

    ui->update->setChecked(updateStableVersion());

    ui->loginHide->setChecked(loginHide());

    //自动登录才能启用
    ui->loginSilent->setEnabled(m_autoLogin);
    ui->loginSilent->setChecked(loginSilent());

    connect(ui->autoRun, &QCheckBox::clicked, this, &Set::setAutoRun, Qt::UniqueConnection);
    connect(ui->loginHide, &QCheckBox::clicked, this, &Set::setLoginHide, Qt::UniqueConnection);
    ui->loginHide->setEnabled(!ui->loginSilent->isChecked());

    ui->cacheDir->setCurrentText(cacheDir());

    ui->path->setText(cacheDir());

    QString machineCode = UserInfo::instance()->returnMachineCode();
    ui->mac->setText(machineCode);
}

void Set::initSetIni()
{
    if (!USERINFO->existAllIni(SetG, "updateStableVersion")) {
        setUpdateStableVersion(true);
    }
}

QString Set::changeLan()
{
    return "zh_cn";
}

bool Set::updateStableVersion()
{
    return USERINFO->readAllIni(SetG, "updateStableVersion").toBool();
}

void Set::setUpdateStableVersion(bool v)
{
    USERINFO->saveAllIni(SetG, "updateStableVersion", v);
}

bool Set::loginHide()
{
    return USERINFO->readAllIni(SetG, "loginHide").toBool();
}

void Set::setLoginHide2(bool v)
{
    USERINFO->saveAllIni(SetG, "loginHide", v);
}

void Set::setLoginHide(bool v)
{
    USERINFO->saveAllIni(SetG, "loginHide", v);
}

bool Set::loginSilent()
{
    return USERINFO->readAllIni(SetG, "loginSilent").toBool();
}

void Set::setLoginSilent(bool v)
{
    USERINFO->saveAllIni(SetG, "loginSilent", v);
}

void Set::setAutoLogin(bool v)
{
    qDebug()<< __FUNCTION__ << v;
    m_autoLogin = v;
    if (!m_autoLogin) {
        setLoginSilent(false);
    }
}

QString Set::cacheDir()
{
    if (m_cacheDir.isEmpty()) {
        m_cacheDir = USERINFO->readAllIni(SetG, "cacheDir").toString();

        if (m_cacheDir.isEmpty()) {
#ifdef Q_OS_WIN
            QStringList list = XFunc::getDistNames();
            list.removeAll("C:");
            m_cacheDir = list.isEmpty() ? "C:" : list.first();
#else
            m_cacheDir = getenv("HOME");
#endif
            m_cacheDir += "\\CGMAGIC";
            setCacheDir(m_cacheDir);
        }
    }
    return m_cacheDir;
}

void Set::setCacheDir(QString v)
{
    QString srcDir = m_cacheDir;
    m_cacheDir = v;
    USERINFO->saveAllIni(SetG, "cacheDir", v);

    if (!QDir(v).exists())
        QDir().mkpath(v);

    if (srcDir != v) {
        QtConcurrent::run([=]{
            qDebug()<< "start copy cache dir" << srcDir << v;
            XFunc::veryCopy(srcDir, v);
            qDebug()<< "end copy cache dir" << srcDir << v;
        });
    }
}


int Set::Langnum()
{
    QString al = USERINFO->readAllIni(SetG, "Language").toString();
    if (al.isEmpty()) {
        al = "zh_cn";
    }
    if (al == "zh_cn") {
        return UserInfo::language_cn;
    } else if (al == "en_us") {
        return UserInfo::language_en;
    } else {
        return UserInfo::language_cn;
    }
}

void Set::setLang()
{
    int L = ui->lang->currentData().toInt();
    QString lang;
    if (L == UserInfo::language_en) {
        lang = "en_us";
    } else if (L == UserInfo::language_cn) {
        lang = "zh_cn";
    }
    USERINFO->saveAllIni(SetG, "Language", lang);//存储
    qDebug()<<"语言改变为"<<lang;
    QTranslator tran;
    bool ret = tran.load(QString(":/language/%1.qm").arg(lang));
    qDebug() << __FUNCTION__ << QString("QTranslator load :/language/%1.qm").arg(lang);
    if (ret) {
        qDebug()<<"安装翻译器";
    } else {
        qDebug() << __FUNCTION__ << QString("QTranslator load :/language/%1.qm error").arg(lang);
    }
    qDebug()<<"翻译ui内容";
}

void Set::setCache()
{
    if (ui->cacheDir->currentText() == cacheDir())
        return;

    setCacheDir(ui->cacheDir->currentText());
}

void Set::on_openCacheDir_clicked()
{
    XFunc::openDir(ui->cacheDir->currentText());
}

void Set::setAutoRun(bool v)
{
    bool ok = USERINFO->AutoRun(v);
    if (!ok) {
        Session::instance()->proExit(2, v);
    }
}
