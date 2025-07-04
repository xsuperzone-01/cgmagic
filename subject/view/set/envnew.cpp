#include "envnew.h"
#include "ui_envnew.h"

#include <QMouseEvent>
#include "tool/xfunc.h"
#include "../../io/pluginlisten.h"
#include "common/basewidget.h"
#include <algorithm>

EnvNew::EnvNew(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnvNew),
    m_envId(0)
{
    ui->setupUi(this);
    ui->widget->hide();
    ui->CoronaBtn->hide();
    ui->vrayBtn->hide();
    ui->fsBtn->hide();
    ui->Coronatext->hide();
    ui->vraytext->hide();
    ui->fsText->hide();

    m_defRendMap.insert("CoronaRender", DefRendererUi(ui->Coronatext, ui->CoronaBtn_2, ui->coronaWid));
    m_defRendMap.insert("vray", DefRendererUi(ui->vraytext, ui->vrayBtn_2, ui->vrayWid));
    m_defRendMap.insert("FStormRender", DefRendererUi(ui->fsText, ui->fsBtn_2, ui->fsWid));
    m_defRendMap.insert("noDefault", DefRendererUi(ui->noText, ui->noBtn_2, ui->noWid));

    QStringList rL = m_defRendMap.keys();

    foreach (QString r, rL) {
        QRadioButton *btn = m_defRendMap.value(r).btn;
        btn->installEventFilter(this);
        connect(btn, &QRadioButton::clicked, this, [=]{
            m_defRendMap.value(r).btn->setChecked(true);
            foreach (QString rr, rL) {
                if (r != rr) {
                    m_defRendMap.value(rr).btn->setChecked(false);
                }
            }
        });
    }

    BaseWidget::setProperty(ui->del, "type", "1");
}

EnvNew::~EnvNew()
{
    delete ui;
}

void EnvNew::initEnvNew(int soft)
{
    ui->soft->clear();
    ui->plugin->clear();
    m_plgMap.clear();
    m_selPlgMap.clear();
    ui->listL->clear();
    ui->listR->clear();

    QJsonObject obj = PluginListen::getPlugins();
    QJsonArray arr;
    QStringList softL;

    if(obj["rows"].isArray())
    {
        arr = obj["rows"].toArray();
    }
    else
    {
        return;
    }

    for(int i = 0; i < arr.size(); i++)
    {
        softL << (arr[i].toObject())["software"].toString();
    }

    if (!softL.isEmpty()) {
        disconnect(ui->soft, SIGNAL(currentIndexChanged(int)), this, SLOT(softChanged(int)));
        ui->soft->clear();
        std::sort(softL.begin(), softL.end());
        ui->soft->addItems(softL);
        QObject::connect(ui->soft, SIGNAL(currentIndexChanged(int)), this, SLOT(softChanged(int)));
        emit ui->soft->currentIndexChanged(0);
        emit softReceive();
    }

    DefaultRendererSet();
}

void EnvNew::setSoft(QJsonObject obj)
{
    ui->soft->setEnabled(false);

    m_envId = 1;

    if (ui->soft->currentText() == obj["software"].toString())
    {
        emit ui->soft->currentIndexChanged(ui->soft->currentIndex());
    }
    else
    {
        ui->soft->setCurrentText(obj["software"].toString());
    }
    setPluginR(obj);
    pluginChanged(ui->soft->currentIndex());
}

void EnvNew::setDefaultSoft(QString soft)
{
    ui->soft->setCurrentText(soft);
}

void EnvNew::setPluginR(QJsonObject obj)
{
    ui->listR->clear();
    QJsonArray plgArr = obj["plugins"].toArray();
    for (int j = 0; j < plgArr.size(); ++j) {
        QJsonObject plg = plgArr.at(j).toObject();
        QString name = plg["name"].toString();
        QString version = plg["version"].toString();
        QString text = name + " " + version;
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setToolTip(text);
        item->setData(Qt::UserRole + 1, name);
        item->setData(Qt::UserRole + 2, version);
        ui->listR->addItem(item);
    }
    ui->listR->sortItems();

    DefaultRendererSet();

    QStringList rL = m_defRendMap.keys();
    foreach (QString r, rL) {
        if (obj.value("DefRenderer").toString().contains(r)) {
            m_defRendMap.value(r).btn->setChecked(true);
        }
    }
}

int EnvNew::envId()
{
    return m_envId;
}

void EnvNew::enableSoft()
{
    ui->soft->setEnabled(true);
}

void EnvNew::softChanged(int idx)
{
    if (ui->soft->currentText().isEmpty())
        return;

    m_plgMap.clear();
    m_selPlgMap.clear();
    ui->plugin->clear();
    ui->listL->clear();
    ui->listR->clear();

    QJsonObject obj = PluginListen::getPlugins();
    QJsonObject plugins;
    for(int i = 0; i < obj["rows"].toArray().size(); i++)
    {
        if(ui->soft->currentText() == obj["rows"].toArray()[i].toObject()["software"].toString())
        {
            QJsonArray arr = obj["rows"].toArray()[i].toObject()["plugins"].toArray();
            QJsonArray plugin;
            for(int j = 0; j < arr.size(); j++)
            {
                QString name = arr.at(j).toObject()["plugin"].toString();
                for(int k = 0; k < arr.at(j).toObject()["version"].toArray().size(); k++)
                {
                    QJsonObject ver;
                    ver["name"] = name;
                    ver["version"] = arr.at(j).toObject()["version"].toArray()[k].toString();
                    plugin.insert(j, ver);
                }
            }
            plugins["plugins"] = plugin;
            break;
        }
    }

    setPlugin(plugins);

    DefaultRendererSet();
}

void EnvNew::setPlugin(QJsonObject obj)
{
    QJsonArray plgArr = obj["plugins"].toArray();

    m_plgMap.clear();

    for (int i = 0; i < plgArr.size(); ++i) {
        QJsonObject plg = plgArr.at(i).toObject();
        QString name = plg["name"].toString();
        QStringList tmpL = m_plgMap.value(name);
        tmpL << plg["version"].toString();
        m_plgMap.insert(name, tmpL);
    }

    QStringList nameL = m_plgMap.keys();
    if (!nameL.isEmpty())
    {
        disconnect(ui->plugin, SIGNAL(currentIndexChanged(int)), this, SLOT(pluginChanged(int)));
        ui->plugin->clear();
        QStringList tmpL;
        for (int i = 0; i < nameL.length(); ++i) {
            QString name = nameL.at(i);
            if (name.isEmpty())
                continue;
            tmpL << name.replace(0, 1, name.at(0).toLower());
        }
        std::sort(tmpL.begin(), tmpL.end());
        QStringList nL;
        for (int i = 0; i < tmpL.length(); ++i) {
            for (int j = 0; j < nameL.length(); ++j) {
                QString name = nameL.at(j);
                if (tmpL.at(i) == name.replace(0, 1, name.at(0).toLower())) {
                    nL << nameL.at(j);
                    nameL.removeAt(j);
                    break;
                }
            }
        }
        ui->plugin->addItems(nL);
        QObject::connect(ui->plugin, SIGNAL(currentIndexChanged(int)), this, SLOT(pluginChanged(int)));
        emit ui->plugin->currentIndexChanged(0);
    }
}

void EnvNew::pluginChanged(int idx)
{

    QString name = ui->plugin->currentText();
    if (name.isEmpty())
        return;

    ui->listL->clear();

    QStringList pL = m_plgMap.value(name);
    QStringList tL = selectText();
    foreach (QString p, pL) {
        QString text = QString("%1 %2").arg(name).arg(p);
        if (!tL.contains(text)) {
            QListWidgetItem * item = new QListWidgetItem(text);
            item->setToolTip(text);
            item->setData(Qt::UserRole + 1, name);
            item->setData(Qt::UserRole + 2, p);
            ui->listL->addItem(item);
        }
    }
    ui->listL->sortItems();
}

QStringList EnvNew::selectName()
{
    QStringList nameL;
    for (int i = 0; i < ui->listR->count(); ++i) {
        QListWidgetItem* item = ui->listR->item(i);
        nameL << item->data(Qt::UserRole + 1).toString();
    }
    return nameL;
}

QStringList EnvNew::selectText()
{
    QStringList textL;
    for (int i = 0; i < ui->listR->count(); ++i) {
        textL << ui->listR->item(i)->text();
    }
    if(ui->listR->count() != 0) {
        BaseWidget::setProperty(ui->del, "type", "1");
    } else {
        BaseWidget::setProperty(ui->del, "type", "0");
    }
    return textL;
}

QString EnvNew::software()
{
    return ui->soft->currentText();
}

QJsonArray EnvNew::plugin()
{
    QJsonArray pa;
    for (int i = 0; i < ui->listR->count(); ++i) {
        QListWidgetItem* item = ui->listR->item(i);
        QJsonObject obj;
        obj.insert("name", item->data(Qt::UserRole + 1).toString());
        obj.insert("version", item->data(Qt::UserRole + 2).toString());
        pa << obj;
    }
    return pa;
}

void EnvNew::clearEnvId()
{
    m_envId = 0;
}

void EnvNew::on_toR_clicked()
{
    QListWidgetItem* item = ui->listL->currentItem();
    if (item) {
        QListWidgetItem* only = NULL;
        for (int i = 0; i < ui->listR->count(); ++i) {
            QListWidgetItem* rt = ui->listR->item(i);
            if (rt->data(Qt::UserRole + 1).toString() == ui->plugin->currentText())
                only = rt;
        }
        if (only) {
            ui->listL->addItem(only->clone());
            ui->listR->takeItem(ui->listR->row(only));
            delete only; only = NULL;
        }

        ui->listR->addItem(item->clone());
        ui->listL->takeItem(ui->listL->currentRow());
        delete item; item = NULL;
    }
    ui->listL->sortItems();
    ui->listR->sortItems();

    bool ret = DefaultRendererSet();
    if(!ret)
    {
        clearBtnEn();
    }
    if(ui->listR->count() != 0) {
        BaseWidget::setProperty(ui->del, "type", "1");
    }
}

void EnvNew::on_toL_clicked()
{
    QListWidgetItem* item = ui->listR->currentItem();
    if (item) {
        QString ps = item->data(Qt::UserRole + 1).toString();
        ui->plugin->setCurrentText(ps);
        ui->listL->addItem(item->clone());
        ui->listR->takeItem(ui->listR->currentRow());
        delete item;
        item = NULL;
    }
    ui->listL->sortItems();

    bool ret = DefaultRendererSet();
    if(!ret)
    {
        clearBtnEn();
    }
    if(ui->listR->count() == 0) {
        BaseWidget::setProperty(ui->del, "type", "0");
    }
}

QString EnvNew::GetDefRenderer()
{
    if (!ui->widget->isHidden()) {
        QStringList rL = m_defRendMap.keys();
        foreach (QString r, rL) {
            if (m_defRendMap.value(r).btn->isChecked()) {
                return r;
            }
        }
    } else {
        QStringList list = selectName();
        QStringList rL = m_defRendMap.keys();
        foreach (QString r, rL) {
            if (list.contains(r)) {
                return r;
            }
        }
    }

    return "";
}

bool EnvNew::DefaultRendererSet()
{
    clearBtnEn();

    QStringList list = selectText();

    int c = 0;
    for (int i = 0; i < list.length(); i++) {
        QString xr = list.at(i);
        QStringList rL = m_defRendMap.keys();
        foreach (QString r, rL) {
            if (xr.contains(r)) {
                c++;
                m_defRendMap.value(r).btn->setText(xr);
                m_defRendMap.value(r).wid->show();
                break;
            }
        }
    }

    if (c > 1) {
        ui->widget->show();
        m_defRendMap.value("noDefault").wid->show();
        m_defRendMap.value("noDefault").btn->setText("不设置");
        return true;
    }

    ui->widget->hide();
    return false;
}

bool EnvNew::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::MouseButtonRelease) {
        if (QMouseEvent *me = static_cast<QMouseEvent *>(e)) {
            if (me->button() == Qt::LeftButton) {
                if (QRadioButton *btn = static_cast<QRadioButton *>(o)) {
                    if (!btn->isChecked())
                        btn->setChecked(true);
                }
            }
        }
    }
    return QWidget::eventFilter(o, e);
}

void EnvNew::clearBtnEn()
{
    QStringList rL = m_defRendMap.keys();
    foreach (QString r, rL) {
        m_defRendMap.value(r).wid->hide();
        m_defRendMap.value(r).btn->setText("");
        m_defRendMap.value(r).btn->setChecked(false);
    }
}

void EnvNew::on_del_clicked()
{
    on_toL_clicked();
}
