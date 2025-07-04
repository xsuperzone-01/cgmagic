#include "activitywidget.h"
#include "ui_activitywidget.h"
#include <QPixmap>
#include <QByteArray>
#include <QEvent>
#include <QDebug>

ActivityWidget::ActivityWidget(BaseWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::ActivityWidget)
{
    ui->setupUi(this);

    setCloseOnFocusOut();
    ui->activityLab1->installEventFilter(this);
    ui->activityLab2->installEventFilter(this);
    ui->activityLab3->installEventFilter(this);
}

ActivityWidget::~ActivityWidget()
{
    delete ui;
}

void ActivityWidget::adjustLabByImageNum(int num){
    if(num == 1){
        ui->activityLab1->show();
        ui->activityLab2->hide();
        ui->activityLab3->hide();
    }else if(num == 2){
        ui->activityLab1->show();
        ui->activityLab2->show();
        ui->activityLab3->hide();
    }else{
        ui->activityLab1->show();
        ui->activityLab2->show();
        ui->activityLab3->show();
    }
}

void ActivityWidget::setImageData(QList<QJsonObject> imageData){
    this->imageData = imageData;
    int num = imageData.size();
    if(num == 0){
        return;
    }else{
        adjustLabByImageNum(num);
    }

    for(int i = 0; i < num; i++){
        QJsonObject imageInfo = imageData.at(i);
        QByteArray imageArray = QByteArray::fromBase64(imageInfo.value("imageByte").toString().toLatin1());
        QPixmap pixmap;
        pixmap.loadFromData(imageArray);
        if(i == 0){
            ui->activityLab1->setPixmap(pixmap);
        }else if(i == 1){
            ui->activityLab2->setPixmap(pixmap);
        }else{
            ui->activityLab3->setPixmap(pixmap);
        }
    }
}

bool ActivityWidget::eventFilter(QObject *watched, QEvent *event){
    if(event->type() == QEvent::MouseButtonPress){
        if(watched == ui->activityLab1 && ui->activityLab1->isVisible() && this->imageData.size() > 0){
            emit jumpUrlInfo(this->imageData.at(0).value("domain").toString(), this->imageData.at(0).value("inAppJump").toInt());
        }else if(watched == ui->activityLab2 && ui->activityLab2->isVisible() && this->imageData.size() > 1){
            emit jumpUrlInfo(this->imageData.at(1).value("domain").toString(), this->imageData.at(1).value("inAppJump").toInt());
        }else if(watched == ui->activityLab3 && ui->activityLab3->isVisible() && this->imageData.size() > 2){
            emit jumpUrlInfo(this->imageData.at(2).value("domain").toString(), this->imageData.at(2).value("inAppJump").toInt());
        }else{
        }
    }else{
    }

    return BaseWidget::eventFilter(watched, event);
}
