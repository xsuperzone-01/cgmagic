#include "preview.h"
#include "ui_preview.h"

#include "common/session.h"
#include "tool/jsonutil.h"
#include "tool/network.h"

#include "QSpacerItem"

Preview::Preview(BaseWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::Preview),
    m_ding(false),
    m_idx(0),
    m_mid(0)
{
    ui->setupUi(this);

    setClass(ui->headClose, "close");
    setClass(ui->headMax, "max");
    setClass(ui->pushButton, "max");

    setAttribute(Qt::WA_Hover, true);//弥补鼠标事件被子窗体获取的问题
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->ding->show();

    ui->viewStack->setCurrentIndex(0);
    ui->loadPro->hide();

    setClass(ui->Pic, "tableDefault");
    setClass(ui->noView, "tableDefaultText");

    ui->headMax->hide();
    setProperty(ui->headMax, "maximize", "maxmax");

    connect(ui->pushButton, &QPushButton::clicked, this, [=](){
        headMaxHand(ui->pushButton);
        loadImage();
    });

}

Preview::~Preview()
{
    qDebug()<< __FUNCTION__;
    delete ui;
}

void Preview::reqView(int missionId, QString target)
{
    ui->image->clear();
    ui->loadPro->setValue(0);

    ui->noView->setText(tr("预览加载中..."));
    ui->viewStack->setCurrentIndex(1);

    ui->title->clear();
    ui->xstate->clear();
    ui->xtime->clear();
    m_save.clear();

    m_mid = missionId;
    m_target = target;

    NET->xrget(QString("/bs/mission/%1/preview/%2?target=%3").arg(m_mid).arg(m_idx).arg(m_target), [=](FuncBody f){
        int status = f.j["status"].toInt();
        if (11001600 == status) {//本地文件
            Session::instance()->mainWid()->m_webTool.openLocalDir(f.j["num"].toString());
            this->close();
        } else {
            show();
            if (11001604 == status) {
                m_preObj = f.j;
                loadView();
            } else {
                showError(f.j["detail"].toString());
            }
        }
    }, this);
}

void Preview::loadView()
{
    if (m_netDown) {
        delete m_netDown; m_netDown = NULL;
    }

    m_netDown = new NetDown(this);
    connect(m_netDown.data(), SIGNAL(showImg()), this, SLOT(loadImage()));

    QString url = m_preObj["url"].toString();
    QString num = m_preObj["num"].toString();
    QString camera = m_preObj["camera"].toString();
    ui->title->setText(QString("%1-%2").arg(num).arg(camera));
    QString pro = m_preObj["progress"].toString();
    if (pro != "-1/-1") {
        int pd = pro.indexOf("[");
        ui->xstate->setText(pro.left(pd));
        ui->xtime->setText(pro.right(pro.length() - pd));
    }

    m_save = USERINFO->tempPath() + QString("/%1-%2.jpg").arg(num).arg(camera);

    QNetworkReply* reply = m_netDown->get(url, m_save);
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
            this, SLOT(loadPro(qint64,qint64)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(error(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(loadImage()));
}

void Preview::showError(QString err)
{
    ui->viewStack->setCurrentIndex(1);
    ui->noView->setText(err.isEmpty() ? tr("暂时无截图预览，请稍后再试~") : err);
    err.isEmpty() ? ui->Pic->show() : ui->Pic->hide();
}


void Preview::resizeEvent(QResizeEvent *e)
{
}

bool Preview::event(QEvent *e)
{
    if (e->type() == QEvent::HoverEnter) {
    }
    if (e->type() == QEvent::HoverMove) {
    }
    if (e->type() == QEvent::HoverLeave) {
    }
    return QWidget::event(e);
}

void Preview::on_dingBtn_clicked()
{
    m_ding = !m_ding;

    QString st = ui->dingBtn->styleSheet();
    m_ding ? st.replace("dingNormal", "dingHover") : st.replace("dingHover", "dingNormal");
    ui->dingBtn->setStyleSheet(st);
}

void Preview::on_right_clicked()
{
    m_idx++;
    reqView(m_mid, m_target);
}

void Preview::on_left_clicked()
{
    m_idx--;
    reqView(m_mid, m_target);
}

void Preview::loadPro(qint64 cur, qint64 total)
{
    qD QString("%1预览进度%2/%3").arg(m_preObj["num"].toString()).arg(cur).arg(total);

    ui->loadPro->show();

    ui->loadPro->setRange(0, total);
    ui->loadPro->setValue(cur);

    if (cur == total)
        ui->loadPro->hide();
}

void Preview::loadImage()
{
    if (m_save.isEmpty())
        return;

    ui->viewStack->setCurrentIndex(0);
    QImage img(m_save);
    if (!img.isNull()) {
        int ww = ui->image->width();
        int wh = ui->image->height();
        if (ww < img.width() || wh < img.height())
            img = img.scaled(ww, wh, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->image->setPixmap(QPixmap::fromImage(img));
    } else {
        showError("");
        QFile ef(m_save);
        ef.open(QFile::ReadWrite);
        QJsonObject obj = JsonUtil::jsonStrToObj(QString(ef.readAll()));
        ef.close();
        if (!obj.isEmpty())
            qD QString("%1预览json%2").arg(m_preObj["num"].toString()).arg(JsonUtil::jsonObjToStr(obj));
    }
}

void Preview::error(QNetworkReply::NetworkError err)
{
    showError("");
    qD QString("%1预览失败%2").arg(m_preObj["num"].toString()).arg(err);
}

void Preview::on_headMax_clicked()
{
    headMaxHand(ui->headMax);
//    ui->headMax->setProperty("maximize", this->isMaximized() ? "maxmax" : "maxmin");
//    ui->headMax->setStyle(qApp->style());
    loadImage();
}
