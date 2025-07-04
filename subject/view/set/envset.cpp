#include "envset.h"
#include "ui_envset.h"

#include "tool/xfunc.h"
//#include "../xrender/xy_define.h"
#include "tool/jsonutil.h"
#include "envblock.h"
#include "resultrule.h"
#include <QGridLayout>
#include <QFileDialog>

#include "common/basewidget.h"
#include <db/userconfig.h>
#include "tool/msgtool.h"
#include "common/basewidget.h"
#include "common/flowlayout.h"
#include "common/eventfilter.h"

#define MaxRenderer "vray|CoronaRender|FStormRender|Krakatoa"

EnvSet::EnvSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnvSet),
    m_type(-1),
    m_projectId(-1)
{
    ui->setupUi(this);

    BaseWidget::setClass(ui->submit, "okBtn");
    BaseWidget::setClass(ui->cancel, "noBtn");

    m_bg.setExclusive(true);
    m_bg.addButton(ui->dmax, xy::rsMax);


    int state = USERINFO->readUserIni("Set", "CustomProjectEn").toInt();
    ui->CustomEn->setChecked(state == 1 ? true : false);

    connect(&m_bg, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(buttonClicked(QAbstractButton*)));

    connect(ui->CustomEn, &QCheckBox::clicked,
            this, &EnvSet::SaveEn);
    emit ui->CustomEn->clicked(ui->CustomEn->isChecked());

    ui->btnWid->hide();

    ui->envNew->hide();

    ui->backWid->hide();

    setBtnStyle(0);

    if (!ui->scrollArea->layout()) {
        ui->scrollArea->setLayout(new FlowLayout(QMargins(0, 0, 0, 0), 16, 12));
        ScrollBarEventFilter *sb = new ScrollBarEventFilter(ui->scrollArea);
        sb->bindEvent(ui->scroll->verticalScrollBar(), ui->scrollArea->layout(), 0);
    }

    ui->backEnvset->setAttribute(Qt::WA_Hover, true);
    ui->backEnvset->installEventFilter(this);
    BaseWidget::setProperty(ui->backEnvset, "type", "0");

}

EnvSet::~EnvSet()
{
    delete ui;
}

void EnvSet::initEnvSet(int project, int type)
{
    m_projectId = project;
    if (-1 == type) {
        type = m_type;
        type = -1==type ? 1 : type;
        m_type = type;
    }

    emit m_bg.buttonClicked(m_bg.button(type));
}

void EnvSet::clearEnvSet()
{
    on_cancel_clicked();
    removeEnvBlock();
    m_rule.clear();
    m_projectId = -1;
}

void EnvSet::buttonClicked(QAbstractButton *btn)
{
    on_cancel_clicked();

    setBtnStyle(btn);

    m_type = m_bg.id(btn);



    m_rule.clear();

    ui->envNew->initEnvNew(m_type);

    removeEnvBlock();
    getEnv();
}

int EnvSet::saveCfg(QJsonObject obj)
{
    if(obj.isEmpty())
        return -1;

    QJsonObject cfg = readProject();
    if(cfg["environment"].toArray().size() == 0)
    {
        cfg["id"] = 0;
        cfg["isDefault"] = 0;
        cfg["plugins"] = obj["plugins"].toArray();
        cfg["projectId"] = obj["projectId"].toInt();
        cfg["software"] = obj["software"].toString();
        cfg["software_type"] = obj["software_type"].toInt();
        cfg["userId"] = 0;
        cfg.insert("DefRenderer", ui->envNew->GetDefRenderer());
        QJsonArray env;
        env.insert(0, cfg);
        QJsonObject pro_cfg;
        pro_cfg["environment"] = env;
        setProject(pro_cfg);
    }
    else
    {
        QJsonObject info;
        QJsonArray env = cfg["environment"].toArray();
        info["id"] = 0;
        info["isDefault"] = 0;
        info["plugins"] = obj["plugins"].toArray();
        info["projectId"] = obj["projectId"].toInt();
        info["software"] = obj["software"].toString();
        info["software_type"] = obj["software_type"].toInt();
        info["userId"] = 0;
        info.insert("DefRenderer", ui->envNew->GetDefRenderer());
        for(int i = 0; i < env.size(); i++)
        {
            if(obj["software"].toString() == env.at(i).toObject()["software"].toString())
            {
                env[i] = info;
                cfg["environment"] = env;
                setProject(cfg);
                return 0;
            }
        }
        env.insert(env.size(), info);
        cfg["environment"] = env;
        setProject(cfg);
    }
    return 0;
}

void EnvSet::addEnv()
{
    QMutexLocker locker(&m_mutex);

    QJsonObject obj;
    obj.insert("projectId", m_projectId);
    obj.insert("software_type", m_type);
    obj.insert("software", ui->envNew->software());
    obj.insert("plugins", ui->envNew->plugin());

    QJsonObject cfg = readProject();
    QJsonArray env = cfg["environment"].toArray();
    for(int i = 0; i < env.size(); i++)
    {
        if(ui->envNew->software() == env.at(i).toObject()["software"].toString())
        {
            MsgTool::msgOk(tr("创建失败, %1 已存在").arg(ui->envNew->software()), this);
            return;
        }
    }

    if(saveCfg(obj) == 0)
    {
        QJsonObject ret = readProject();
        showEnv(ret);
        QJsonArray envArr = ret["environment"].toArray();
        for (int i = 0; i < envArr.size(); ++i) {
            QJsonObject env = envArr.at(i).toObject();
            if (1 == env["isDefault"].toInt()) {
                USERINFO->saveUserIni("EnvPre", QString("defaultEnvId_%1").arg(m_type), env["id"].toInt());
                emit refreshEnvPre(m_projectId);
            }
        }
    }
    else
    {
        MsgTool::msgOk(tr("创建失败"), this);
    }
}

void EnvSet::updateEnv(int id)
{
    QMutexLocker locker(&m_mutex);
    QJsonObject obj;
    obj.insert("projectId", m_projectId);
    obj.insert("id", id);
    obj.insert("software_type", m_type);
    obj.insert("software", ui->envNew->software());
    obj.insert("plugins", ui->envNew->plugin());
    if(saveCfg(obj) == 0)
    {
        on_cancel_clicked();
        showEnv(readProject());
        emit refreshEnvPre(m_projectId);
    }
    else
    {
        MsgTool::msgOk(tr("修改失败"), this);
    }
}

int EnvSet::delCfg(QString software)
{
    QJsonObject cfg = readProject();
    if(!cfg.isEmpty())
    {
        QJsonArray env = cfg["environment"].toArray();
        for(int i = 0; i < env.size(); i++)
        {
            if(software == env.at(i).toObject()["software"].toString())
            {
                env.removeAt(i);
                cfg["environment"] = env;
                setProject(cfg);
                break;
            }
        }
    }

    return 0;
}

void EnvSet::delEnv(int id, QString software)
{
    QMutexLocker locker(&m_mutex);
    QJsonObject obj;
    obj.insert("projectId", m_projectId);
    obj.insert("id", id);
    obj.insert("software_type", m_type);

    delCfg(software);

    showEnv(readProject());
    emit refreshEnvPre(m_projectId);
}

void EnvSet::showEnv(QJsonObject obj)
{
    if (FlowLayout* grid = (FlowLayout*)ui->scrollArea->layout()) {

        removeEnvBlock();

        int brow = 0;
        int bcol = 0;

        QJsonArray envArr = obj["environment"].toArray();
        QJsonObject obj;
        obj.insert("software", "");
        envArr.append(obj);
        if (1 == envArr.size()) {
            QJsonObject obj;
            obj.insert("software", "software");
            obj.insert("env_block", 1);
            envArr.append(obj);
        }


        for (int i = 0; i < envArr.size(); ++i) {
            QJsonObject env = envArr.at(i).toObject();
            QStringList rL;
            QStringList pL;
            QJsonArray plgArr = env["plugins"].toArray();
            QString DefRenderer = env.value("DefRenderer").toString();
            for (int j = 0; j < plgArr.size(); ++j) {
                QJsonObject plg = plgArr.at(j).toObject();
                QString name = plg.value("name").toString();

                QString text = name + " " + plg["version"].toString();
                if (!DefRenderer.isEmpty() && name.contains(DefRenderer)) {
                    text += tr(" (默认渲染器)");

                }
                if (name.contains(QRegularExpression(MaxRenderer)))
                    rL << text;
                else
                    pL << text;
            }

            QString soft = env["software"].toString();

            EnvBlock* block = new EnvBlock(ui->scrollArea);
            connect(block, SIGNAL(envBlockHand(int,QString)), this, SLOT(envSetHand(int,QString)));
            block->setFixedSize(192, 224);
            if (env.contains("env_block"))
                block->removeStack();
            else {
                qDebug()<< "111" << rL << pL;
                block->initEnvBlock(soft.isEmpty(), soft, rL, pL, JsonUtil::jsonObjToStr(env), env["isDefault"].toInt());
            }
            grid->addWidget(block);
        }

    }

    if (obj.contains("projectSoftwareConfig")) {
        QJsonObject con = obj["projectSoftwareConfig"].toObject();
        QString output = con["outputPath"].toString();
        emit refreshResultDir(output);
        m_rule = con["extraConfig"].toString();

        QString key = QString("%1_%2").arg(m_projectId).arg(m_type);
        XFunc::m_projectConfig.insert(key, con);
    }
}

bool EnvSet::validateEnv()
{
    bool hasXR = false;
    QStringList nameL = ui->envNew->selectName();

    for (int i = 0; i < nameL.length(); ++i) {
        QString name = nameL.at(i);
        if (xy::rsMaya == m_type) {
            name = name.toLower();
            if (name.contains(QRegularExpression("vrayformaya|mtoa|redshift|renderman"))) {
                hasXR = true;
                break;
            }
        }
        if (xy::rsMax == m_type) {
            if (name.contains(QRegularExpression(MaxRenderer))) {
                hasXR = true;
                break;
            }
        }
    }

    if ((xy::rsMaya == m_type || xy::rsMax == m_type) && !hasXR) {
        if (0 == MsgTool::msgChooseLoop(tr("未选择渲染器"), this))
            return true;
        else
            return false;
    }
    else
    {
        QString defRen = ui->envNew->GetDefRenderer();
        return true;
    }
}

void EnvSet::setBtnStyle(QAbstractButton* btn)
{
    QList<QAbstractButton*> btnL = m_bg.buttons();
    for (int i = 0; i < btnL.length(); ++i) {
        btnL.at(i)->setProperty("tab", btnL.at(i) != btn ? "0" : "1");
        btnL.at(i)->setStyle(qApp->style());
    }
}

void EnvSet::updatePath()
{
    QJsonObject obj;
    obj.insert("projectId", m_projectId);
    obj.insert("software_type", m_type);
    obj.insert("extraConfig", m_rule);
 /*   TCP->xrs(105, 10509, obj, [=](int val, QJsonDocument doc){
        getEnv();
    }, this);*/
}

void EnvSet::getEnv()
{
    QJsonObject user;
    user.insert("projectId", m_projectId);
    user.insert("software_type", m_type);

    showEnv(readProject());
}

int EnvSet::defaultCfg(QString software)
{
    qD software;
    return 0;
}

void EnvSet::defaultEnv(int id, QString software)
{
    QMutexLocker locker(&m_mutex);
    QJsonObject user;
    user.insert("projectId", m_projectId);
    user.insert("id", id);
    user.insert("software_type", m_type);

    defaultCfg(software);
    showEnv(readProject());
    emit refreshEnvPre(m_projectId);
}

void EnvSet::removeEnvBlock()
{
    FlowLayout* grid = (FlowLayout*)ui->scrollArea->layout();
    QList<EnvBlock*> envL = ui->scrollArea->findChildren<EnvBlock*>();
    for (int i = 0; i < envL.length(); ++i) {
        EnvBlock* wid = envL.at(i);
        grid->removeWidget(wid);
        delete wid; wid = NULL;
    }
}

void EnvSet::on_cancel_clicked()
{
    ui->btnWid->hide();
    ui->envBlockWid->show();
    ui->envNew->enableSoft();
    ui->envNew->hide();
    ui->envNew->clearEnvId();
    ui->envNew->initEnvNew(m_type);
//    if(ui->widget->isVisible()){
//    }else {
//    ui->widget->show();
//}
    ui->widget->show();
    ui->Wid->show();
    ui->checkWid->show();
    ui->backWid->hide();
}

void EnvSet::envSetHand(int type, QString json)
{

    QJsonObject obj = JsonUtil::jsonStrToObj(json);
    if (0 == type) {//default
        defaultEnv(obj["id"].toInt(), obj["software"].toString());
    }

    if (1 == type) {//mod
        ui->btnWid->show();
        ui->envBlockWid->hide();
        ui->envNew->show();
        ui->envNew->setSoft(obj);
        ui->Wid->hide();
        ui->checkWid->hide();
        ui->widget->hide();
        ui->backWid->show();
    }
    if (2 == type) {//del
        delEnv(obj["id"].toInt(), obj["software"].toString());
    }
    if (3 == type) {//add
        ui->envNew->clearBtnEn();
        ui->envNew->show();
        ui->btnWid->show();
        ui->envBlockWid->hide();
        ui->widget->hide();
        ui->Wid->hide();
        ui->checkWid->hide();
        ui->backWid->show();
    }
}

void EnvSet::on_submit_clicked()
{
    if (validateEnv()) {
        int envId = ui->envNew->envId();
        if (envId)
            updateEnv(envId);
        else
            addEnv();

        ui->widget->show();
        ui->Wid->show();
        ui->checkWid->show();
        ui->envNew->hide();
        ui->envNew->clearEnvId();
        ui->envNew->initEnvNew(m_type);
        ui->btnWid->hide();
        ui->envBlockWid->show();
        ui->backWid->hide();
    }
}

void EnvSet::on_projectOpen_clicked()
{
    {
        updatePath();
    }
}

void EnvSet::on_resultOpen_clicked()
{
    {
        updatePath();
    }
}

void EnvSet::on_advance_clicked()
{
    ResultRule* rule = new ResultRule(this);
    connect(rule, SIGNAL(resultRule(QString)), this, SLOT(resultRule(QString)));
    rule->initResultRule(m_rule);
}

void EnvSet::resultRule(QString rule)
{
    qD __FUNCTION__  << m_rule;
    m_rule = rule;
    qD __FUNCTION__ << rule;
    updatePath();
}

void EnvSet::SaveEn(bool state)
{
    USERINFO->saveUserIni("Set", "CustomProjectEn", state == true ? 1 : 0);
    ui->widget_2->setEnabled(ui->CustomEn->isChecked());

}

void EnvSet::setProject(QJsonObject &project)
{
    QStringList pmL;
    QJsonArray env = project["environment"].toArray();
    foreach (QJsonValue val, env) {
        QJsonObject obj = val.toObject();
        QString def = obj["DefRenderer"].toString();
        QString defver;
        QJsonArray plg = obj["plugins"].toArray();
        foreach (QJsonValue val, plg) {
            QJsonObject po = val.toObject();
            if (po["name"].toString() == def) {
                defver = po["version"].toString();
                break;
            }
        }
        pmL << QString("DefRenderer:%1_%2,software:%3").arg(def).arg(defver).arg(obj["software"].toString());
    }
    QString pm = pmL.join("|");
    qD pm;
    USERINFO->saveUserIni("Set", "ProjectManager", pm);
    USERINFO->saveUserIni("Set", "ProjectManager2", JsonUtil::jsonObjToStr(project));
}

QJsonObject EnvSet::readProject()
{
    return JsonUtil::jsonStrToObj(USERINFO->readUserIni("Set", "ProjectManager2").toString());
}

void EnvSet::on_backEnvset_clicked()
{
    on_cancel_clicked();
}

void EnvSet::on_backEnvset_2_clicked()
{
    on_backEnvset_clicked();
}

bool EnvSet::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->backEnvset){
        if (event->type() == QEvent::HoverEnter){
            BaseWidget::setProperty(ui->backEnvset, "type", "1");
        }
        if (event->type() == QEvent::MouseButtonPress){
            BaseWidget::setProperty(ui->backEnvset, "type", "2");
        }
        if (event->type() == QEvent::HoverLeave){
            BaseWidget::setProperty(ui->backEnvset, "type", "0");
        }
    }
    return QWidget::eventFilter(watched, event);
}
