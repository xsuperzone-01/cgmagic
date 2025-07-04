#include "firstrun.h"
#include "ui_firstrun.h"
#include "config/userinfo.h"
#include <QDebug>
#include "view/set/set.h"

FirstRun::FirstRun(QWidget *parent) :
    BaseWidget(parent),
    ui(new Ui::FirstRun)
{
    ui->setupUi(this);
    {
        selfAdaptionFixedSize();
        selfAdaptionMargins();

        ui->imgWidget->selfAdaptionMargins();
        ui->widget->selfAdaptionMargins();
    }
    ui->paintWidget->installEventFilter(this);



    modaldel();//模态化

    raise();
    activateWindow();

    if (parent) {
        connect(parent, SIGNAL(destroyed()), this, SLOT(deleteLater()));
        QPoint gp = parent->mapToGlobal(QPoint(0,0));
        move(gp.x(), gp.y());
        resize(parent->size());
    }

    setClass(ui->nextBtn, "nextBtn");

    m_index = 0;
    ui->nextBtn->setText(tr("下一步"));

    QString lan = Set::changeLan();
    if(lan == "en_us"){
        setText(tr("跳过(1/9)"), tr("全新导航设计"), tr("CGMAGIC主页升级为客户端，新增云渲染模块。"), 273, 83, 261, 162);//注：宽度结果像素级调整，一个像素都无法缩小

        connect(ui->nextBtn, &QPushButton::clicked, [=](bool clicked){
            m_index++;
            switch (m_index) {
            case 1:
                setText(tr("跳过(2/9)"), tr("顶部导航栏升级"), tr("付费入口、客服、帮助中心、设置都在这里哦~"),
                        856, 87, 464, 122);
                break;
            case 2:
                setText(tr("跳过(3/9)"), tr("云转模提交文件"), tr("点击这里可以提交文件进行模型版本转换哦~"),
                        819, 686, 375, 122);
                break;
            case 3:
                setText(tr("跳过(4/9)"), tr("云转模设置和云渲染设置"), tr("想要对云转模和云渲染进行单独设置可以点击这里哦~"),
                        275, 105, 559, 122);
                break;
            case 4:{
                setText(tr("跳过(5/9)"), tr("任务列表界面"), tr("在这里可以看到任务相关的数据哦~"),
                        25, 482, 229, 142);
                break;
            }
            case 5:
                setText(tr("跳过(6/9)"), tr("付费会员信息查看"), tr("点击顶部导航栏上方头像区域，即可在弹框内看到付费会员相关信息，可快捷跳转付费页面。"),
                        402, 136, 276, 182);
                break;
            case 6:
                setText(tr("跳过(7/9)"), tr("搜索框"), tr("在任务管理界面和下载界面可以点击列表上方搜索框对任务名进行搜索，没有搜索框的列表不支持搜索。"),
                        283, 150, 362, 162);
                break;
            case 7:
                setText(tr("跳过(8/9)"), tr("通用设置"), tr("通用设置、软件更新、关于入口都在这里哦~"),
                        1142, 83, 269, 142);
                break;
            case 8:
                setProperty(ui->nextBtn, "type", "true");
                ui->nextBtn->setText(tr("开始体验"));
                setText(tr("跳过(9/9)"), tr("在线客服/帮助中心"), tr("使用过程中，如果遇到问题，可以直接点击联系客服按钮或跳转网页个人中心解决哦~"),
                        957, 94, 428, 162);
                break;
            case 9:{
                USERINFO->instance()->setFirstRun(1);
                close();
                break;
            }
            default:
                break;
            }
        });
    }else{
        setText(tr("跳过(1/3)"), tr("帮助中心"), tr("任何功能使用问题前往帮助中心查看详细教程~"), 666, 80, 302, 122);//注：宽度结果像素级调整，一个像素都无法缩小

        connect(ui->nextBtn, &QPushButton::clicked, [=](bool clicked){
            m_index++;
            switch (m_index) {
            case 1:
                setText(tr("跳过(2/3)"), tr("活动中心"), tr("活动福利详情以及公告都在这里~"),
                        666, 80, 289, 122);
                break;
            case 2:
                setText(tr("跳过(3/3)"), tr("个人账户中心"), tr("个人账户信息请到这里查看"),
                        330, 24, 277, 122);
                break;
            // case 3:
            //     setText(tr("跳过(4/9)"), tr("云转模设置和云渲染设置"), tr("想要对云转模和云渲染进行单独设置可以点击这里哦~"),
            //             275, 105, 325, 122);
            //     break;
            // case 4:{
            //     setText(tr("跳过(5/9)"), tr("任务列表界面"), tr("在这里可以看到任务相关的数据哦~"),
            //             25, 482, 229, 122);
            //     break;
            // }
            // case 5:
            //     setText(tr("跳过(6/9)"), tr("付费会员信息查看"), tr("点击顶部导航栏上方头像区域，即可在弹框内看到付费会员相关信息，可快捷跳转付费页面。"),
            //             713, 114, 268, 162);
            //     break;
            // case 6:
            //     setText(tr("跳过(7/9)"), tr("搜索框"), tr("在任务管理界面和下载界面可以点击列表上方搜索框对任务名进行搜索，没有搜索框的列表不支持搜索。"),
            //             283, 150, 282, 162);
            //     break;
            // case 7:
            //     setText(tr("跳过(8/9)"), tr("通用设置"), tr("通用设置、软件更新、关于入口都在这里哦~"),
            //             1142, 83, 278, 122);
            //     break;
            // case 8:
            //     setProperty(ui->nextBtn, "type", "true");
            //     ui->nextBtn->setText(tr("开始体验"));
            //     setText(tr("跳过(9/9)"), tr("在线客服/帮助中心"), tr("使用过程中，如果遇到问题，可以直接点击联系客服按钮或跳转网页个人中心解决哦~"),
            //             1093, 81, 293, 142);
            //     break;
            case 3:{
                USERINFO->instance()->setFirstRun(1);
                close();
                break;
            }
            default:
                break;
            }
     });
    }

    connect(ui->finishBtn, &QPushButton::clicked, [=](bool clicked){
        USERINFO->instance()->setFirstRun(1);
        close();
    });

    show();
}

void FirstRun::setText(QString a, QString b, QString c, int i, int j, int k, int l)
{
    ui->finishBtn->setText(a);//tr(a);报错
    ui->title->setText(b);
    ui->content->setText(c);//1093,81

    ui->content->setWordWrapMode(QTextOption::WrapAnywhere);
    BaseWidget::setLineHeight(ui->content, 22);

    ui->paintWidget->move(i - 6,j);
    ui->paintWidget->setFixedSize(k + 12,l + 12);
    ui->imgWidget->setFixedSize(k, l);

    loadImage();
}


FirstRun::~FirstRun()
{
    delete ui;
}

bool FirstRun::needMove(QMouseEvent *e)
{
    return false;//禁止移动
}

void FirstRun::loadImage()
{
    QString lan = Set::changeLan();
    QString res = QString("QWidget#backWidget{border-image:url(:/first/%1/%2.png);}")
            .arg(lan).arg(m_index);
    ui->backWidget->setStyleSheet(res);
}


bool FirstRun::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->paintWidget && event->type() == QEvent::Paint)
       {
           showPaint(); //响应函数
       }
       return QWidget::eventFilter(watched,event);
}


void FirstRun::showPaint()
{
    QPainter painter(ui->paintWidget);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);//消锯齿
    painter.setPen(Qt::NoPen);

    QColor color(44, 45, 49);
    QBrush brush(color);
    painter.setBrush(brush);//封闭区域填充

        QPointF position(6, 6);//发现绘制图像被背景图遮挡

        switch (m_index) {
        case 0:{//左三角
            QPointF points[3] = {
                QPointF(position.x() - 6, position.y() + 55 + 6),
                QPointF(position.x(), position.y() + 55 + 12),
                QPointF(position.x(), position.y() + 55)
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 1:{//上三角
            QString lan = Set::changeLan();
            QPointF points[3] = {
                QPointF(position.x() + (lan == "en_us" ? 232 : 130) + 6, position.y() - 6),
                QPointF(position.x() +(lan == "en_us" ? 232 : 130), position.y()),
                QPointF(position.x() + (lan == "en_us" ? 232 : 130) + 12, position.y())
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);

            break;
        }
        case 2:{//右三角
            QString lan = Set::changeLan();
            QPointF points[3] = {
                QPointF(position.x() + (lan == "en_us" ? 375 : 277)*double(width())/1440 + 6, position.y() + 55),
                QPointF(position.x() + (lan == "en_us" ? 375 : 277)*double(width())/1440, position.y() + 55 + 6),
                QPointF(position.x() + (lan == "en_us" ? 375 : 277)*double(width())/1440, position.y() + 55 - 6)
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 3:{//左三角
            QPointF points[3] = {
                QPointF(position.x() - 6, position.y() + 6 + 55),
                QPointF(position.x() , position.y() + 55),
                QPointF(position.x() , position.y() + 12 + 55)
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 4:{//右三角
            QPointF points[3] = {
                QPointF(position.x() + 229*double(width())/1440 + 6, position.y() + 55),
                QPointF(position.x() + 229*double(width())/1440, position.y() + 55 + 6),
                QPointF(position.x() + 229*double(width())/1440, position.y() + 55 - 6)
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 5:{//左三角
            QString lan = Set::changeLan();
            QPointF points[3] = {
                QPointF(position.x() - 6, position.y() + 55 + 6),
                QPointF(position.x(), position.y() + 55 + 12),
                QPointF(position.x(), position.y() + 55)
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 6:{//上三角
            QPointF points[3] = {
                QPointF(position.x() + 130 + 6, position.y() - 6),
                QPointF(position.x() + 130, position.y()),
                QPointF(position.x() + 130 + 12, position.y())
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 7:{//上三角
            QPointF points[3] = {
                QPointF(position.x() + 130 + 6, position.y() - 6),
                QPointF(position.x() + 130, position.y()),
                QPointF(position.x() + 130 + 12, position.y())
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        case 8:{//上三角
            QString lan = Set::changeLan();
            QPointF points[3] = {
                QPointF(position.x() + (lan == "en_us" ? 269 : 130) + 6, position.y() - 6),
                QPointF(position.x() + (lan == "en_us" ? 269 : 130), position.y()),
                QPointF(position.x() + (lan == "en_us" ? 269 : 130) + 12, position.y())
            };
            QPen pen(Qt::NoPen);
            painter.setPen(pen);
            painter.drawPolygon(points, 3);
            break;
        }
        default:
            break;
}

        painter.end();

        this->update();
}
