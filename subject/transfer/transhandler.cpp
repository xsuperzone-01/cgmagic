#include "transhandler.h"

#include <QEventLoop>
#include <QPointer>

#include "tool/jsonutil.h"
#include "tool/xfunc.h"
#include "tool/network.h"

TransHandler::TransHandler(QObject *parent) :
    QObject(parent)
{
    m_FsIps.clear();
}

DownloadUrl TransHandler::downloadLink(int farmId, int fileId)
{
    DownloadUrl link;

    QPointer<QEventLoop> loop = new QEventLoop(this);
    QTimer::singleShot(5000, loop, SLOT(quit()));

    NET->xrget(QString("/bs/mission/%1/results/downloadUrl?farmId=%2").arg(fileId).arg(farmId), [=, &link](FuncBody f){
        if (!loop) return;
        loop->quit();
        link.status = f.j.value("status").toInt();
        link.url = f.j.value("url").toString();
    }, this);

    loop->exec();
    loop->deleteLater();

    return link;
}

Farm TransHandler::farmInfo(int farmId, int fileId)
{
    Farm farm;
    if (m_FsIps.contains(farmId))
    {
        QByteArray bytes = m_FsIps.value(farmId);
        if (!bytes.isEmpty())
        {
            QJsonObject obj = JsonUtil::jsonStrToObj(QString::fromUtf8(bytes));
            farm.error = obj.value("status").toInt();
            if(farm.error == 11040000 && obj.value("data").toArray().size() != 0)
            {
                int index = fileId % obj.value("data").toArray().size();
                if(index >= 0)
                {
                    farm.id = farmId;
                    farm.error = 0;
                    farm.domain = obj.value("data").toArray().at(index).toObject().value("domain").toString();
                    farm.ip = XFunc::ipByDomain(farm.domain);
                    farm.port = obj.value("data").toArray().at(index).toObject().value("port").toInt();
                }
                qD __FUNCTION__ << "farmId = " << farmId << "fileId = " << fileId;
                qD __FUNCTION__ << "fs的ip组:" << bytes;
                qD __FUNCTION__ << "fs的ip组个数 = " << obj.value("data").toArray().size();
                qD __FUNCTION__ << "index = " << index << "ip = " << farm.ip << "port = " << farm.port;
            }
        }
    }

    if (farm.id == -1) {
        farm.error = 10010;
        QPointer<QEventLoop> loop = new QEventLoop(this);
        QTimer::singleShot(5000, loop, SLOT(quit()));

        NET->xrget(QString("/bs/farm/%1").arg(farmId), [=,&farm](FuncBody f){
            if (!loop) return;
            loop->quit();

            farm.id = farmId;
            qD __FUNCTION__ << "farmId = " << farmId << "fileId = " << fileId;
            if (!f.j.value("domain").toString().isEmpty())
                GetFsIp(QString("http://%1:%2/GetFsIp").arg(XFunc::ipByDomain(f.j.value("domain").toString())).arg(f.j.value("port").toInt()),farm, fileId);
        }, this);

        loop->exec();
        loop->deleteLater();
    }

    return farm;
}

int TransHandler::missionUpState(QString num)
{
    int code = 10010;//默认超时码

    QPointer<QEventLoop> loop = new QEventLoop(this);
    QTimer::singleShot(5000, loop, SLOT(quit()));

    NET->xrget(QString("/bs/mission/%1/upload/right").arg(num), [=,&code](FuncBody f){
        if (!loop) return;
        loop->quit();
        code = f.j.value("status").toInt();
    }, this);

    loop->exec();
    loop->deleteLater();

    if (code == 11001400)
        code = TransSet::missionUp;
    if (code == 11001401)
        code = TransSet::missionNoUp;
    return code;
}

bool TransHandler::reTrans(QString key)
{
    int count = m_reTrans.value(key, 1);
    if (count == 10) {
        m_reTrans.take(key);
    } else {
        count++;
        m_reTrans.insert(key, count);
    }
    return m_reTrans.contains(key);
}

void TransHandler::GetFsIp(const QString url, Farm &farm, const int fileId)
{
    qDebug() << "GetFsIp url = " << url << "fileId = " << fileId;
    QNetworkRequest request;
    QNetworkAccessManager *naManager = new QNetworkAccessManager(this);
    request.setUrl(QUrl(url));
    QNetworkReply *reply = naManager->get(request);

    QEventLoop temp_loop;
    QObject::connect(reply, SIGNAL(finished()), &temp_loop, SLOT(quit()));
    temp_loop.exec();

    QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if((statusCodeV.toInt() == 200) && (reply->error() == QNetworkReply::NoError))
    {
        QByteArray bytes = reply->readAll();
        QJsonObject obj = JsonUtil::jsonStrToObj(QString::fromUtf8(bytes));
        qDebug() << "GetFsIp = " << obj;
        farm.error = obj.value("status").toInt();
        if(farm.error == 11040000 && obj.value("data").toArray().size() != 0)
        {
            int index = fileId % obj.value("data").toArray().size();
            if(index >= 0)
            {
                farm.error = 0;
                farm.domain = obj.value("data").toArray().at(index).toObject().value("domain").toString();
                farm.ip = XFunc::ipByDomain(farm.domain);
                farm.port = obj.value("data").toArray().at(index).toObject().value("port").toInt();
            }
            qD __FUNCTION__ << "fs的ip组:" << bytes;
            qD __FUNCTION__ << "fs的ip组个数 = " << obj.value("data").toArray().size();
            qD __FUNCTION__ << "index = " << index << "ip = " << farm.ip << "port = " << farm.port;
            m_FsIps.insert(farm.id, bytes);
        }
    }
    else
        qDebug() << "GetFsIp fail code ="  << statusCodeV.toInt() << "error = " << reply->error();

    reply->deleteLater();
    naManager->deleteLater();
}

void TransHandler::removeFs(int farmId)
{
    m_FsIps.remove(farmId);
}
