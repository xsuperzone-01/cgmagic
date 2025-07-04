#include "noticewidget.h"
#include "ui_noticewidget.h"
#include <QMutex>
#include <QMutexLocker>
#include "config/userinfo.h"

NoticeWidget* NoticeWidget::noticeWidget = nullptr;

NoticeWidget::NoticeWidget(BaseWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::NoticeWidget),
    mousePress(false)
{
    ui->setupUi(this);

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setFixedSize(QSize(864,562));
}

NoticeWidget::~NoticeWidget()
{
    delete ui;
    noticeWidget = nullptr;
}

//生成单例对象
NoticeWidget* NoticeWidget::getInstance(){
    if(!noticeWidget){
        static QMutex mutex;
        QMutexLocker locker(&mutex);

        if(!noticeWidget){
            noticeWidget = new NoticeWidget;
        }
    }

    return noticeWidget;
}

//窗口显示
void NoticeWidget::showWindow(){
    modaldel();
    show();
}

//加载网页
void NoticeWidget::addWebPage(QString url){
    QPointer<WebView> webPage = new WebView(this);
    QString webUrl = url;
    webPage->load(webUrl);
    ui->webPage->layout()->addWidget(webPage);
}


//窗口关闭槽函数
void NoticeWidget::on_closeBtn_clicked()
{
    this->close();
}

void NoticeWidget::mousePressEvent(QMouseEvent *event){
    if((event->button() == Qt::LeftButton)){
        mousePress = true;
        mousePoint = event->globalPos() - this->pos();
    }else{
    }
}

void NoticeWidget::mouseReleaseEvent(QMouseEvent *event){
    mousePress = false;
}

void NoticeWidget::mouseMoveEvent(QMouseEvent *event){
    if(mousePress){
        move(event->globalPos() - mousePoint);
    }else{}
    this->update();
}
