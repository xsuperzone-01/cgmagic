#include "envblock.h"
#include "ui_envblock.h"

#include "config/userinfo.h"
#include "common/basewidget.h"
#include <QDebug>

EnvBlock::EnvBlock(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnvBlock)
{
    ui->setupUi(this);
    ui->setDefaultWid->hide();

    ui->addEnv->setAttribute(Qt::WA_Hover, true);
    ui->addEnv->installEventFilter(this);

    if (!this->isEnabled())
        BaseWidget::setProperty(ui->addEnv, "type", "3");
}

EnvBlock::~EnvBlock()
{
    delete ui;
}

void EnvBlock::initEnvBlock(bool add, QString soft, QStringList rdL, QStringList plgL, QString json, bool isDefault)
{
    if (add)
        ui->stack->setCurrentIndex(1);
    else {
        ui->stack->setCurrentIndex(0);

        ui->soft->setText(soft);
        foreach (QString plg, plgL) {
        }
        ui->defaultLabel->setVisible(isDefault);
        ui->setDefault->setEnabled(!isDefault);

        QVBoxLayout *lay = (QVBoxLayout *)ui->scrollAreaWidgetContents->layout();

        QString suffix = " (默认渲染器)";//中文版（条件判断）  // (Default Renderer)英文版
        QString suffix_en = " (Default Renderer)";

        QList<QStringList> listL;
        listL << rdL << plgL;
        for (int i = 0; i < listL.length(); i++) {
            QStringList list = listL.at(i);
            if (!list.isEmpty()) {
                QLabel *renderer = new QLabel(this);
                renderer->setText(0 == i ? tr("渲染器") : tr("插件"));
                BaseWidget::setClass(renderer, "envBlockPlugin");
                BaseWidget::setProperty(renderer, "title", true);
                lay->addWidget(renderer);

                foreach (QString text, list) {
                    QLabel *renderer = new QLabel(this);
                    if(text.endsWith(suffix)){
                        int suffixLength = suffix.length();
                        text = text.chopped(suffixLength);
                        QHBoxLayout *hlay = new QHBoxLayout(this);
                        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred);

                        QLabel *def = new QLabel(this);
                        def->setStyleSheet("border-image: url(:/rendefault.png);");
                        def->setFixedSize(36, 16);
                        renderer->setText(text);
                        BaseWidget::setClass(renderer, "envBlockPlugin");
                        hlay->addWidget(renderer);
                        hlay->addWidget(def);
                        hlay->addSpacerItem(spacer);
                        lay->addLayout(hlay);
                    }else if(text.endsWith(suffix_en)) {
                        int suffixLength = suffix_en.length();
                        text = text.chopped(suffixLength);
                        QHBoxLayout *hlay = new QHBoxLayout(this);
                        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred);

                        QLabel *def = new QLabel(this);
                        def->setStyleSheet("border-image: url(:/rendefault_en.png);");
                        def->setFixedSize(48, 16);
                        renderer->setText(text);
                        BaseWidget::setClass(renderer, "envBlockPlugin");
                        hlay->addWidget(renderer);
                        hlay->addWidget(def);
                        hlay->addSpacerItem(spacer);
                        lay->addLayout(hlay);
                    }else {
                    renderer->setText(text);
                    BaseWidget::setClass(renderer, "envBlockPlugin");
                    lay->addWidget(renderer);
                    }
                }
            }
        }

        lay->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    }

    m_json = json;
}

void EnvBlock::removeStack()
{
    ui->stack->deleteLater();
}

void EnvBlock::on_setDefault_clicked()
{
    emit envBlockHand(0, m_json);
}

void EnvBlock::on_mod_clicked()
{
    emit envBlockHand(1, m_json);
}

void EnvBlock::on_del_clicked()
{
    emit envBlockHand(2, m_json);
}

void EnvBlock::on_addEnv_clicked()
{
    emit envBlockHand(3, m_json);
}

void EnvBlock::on_addEnvIcon_clicked()
{
    on_addEnv_clicked();
}

bool EnvBlock::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->addEnv){
        if ( this->isEnabled()) {
            if (event->type() == QEvent::HoverEnter){
                BaseWidget::setProperty(ui->addEnv, "type", "1");
            }
            if (event->type() == QEvent::MouseButtonPress){
                BaseWidget::setProperty(ui->addEnv, "type", "2");
            }
            if (event->type() == QEvent::HoverLeave){
                BaseWidget::setProperty(ui->addEnv, "type", "0");
            }
        }
        if (event->type() == QEvent::EnabledChange) {
            if (ui->addEnv->isEnabled()) {
                BaseWidget::setProperty(ui->addEnv, "type", "0");
            } else {
                BaseWidget::setProperty(ui->addEnv, "type", "3");
            }
        }
        }

    return QWidget::eventFilter(watched, event);
}
