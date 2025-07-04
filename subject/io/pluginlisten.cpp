#include "pluginlisten.h"

#include "tool/xfunc.h"
#include <QSettings>
#include "common/trayicon.h"
#include "common/session.h"
#include "view/msgbox.h"

#include <QTextStream>
#include "tool/jsonutil.h"
#include "view/set/envset.h"
#include <db/userconfig.h>
#include "tool/network.h"
#include "db/downloaddao.h"
#include "db/uploaddao.h"
#include "view/set/renderset.h"
#include "tool/regedit.h"
#include "plugin/plugincorrespond.h"

QJsonObject PluginListen::m_plugin;
QStringList PluginListen::m_unityVersion;
QJsonObject PluginListen::m_sketchVersion;
QJsonObject PluginListen::m_Max_plugins;
QMap<QString, int> PluginListen::vrverMap;

PluginListen::PluginListen(QObject *parent) :
    QObject(parent),
    m_server(NULL),
    m_unityServer(NULL),
    m_sketchServer(NULL)
{

}

PluginListen::~PluginListen()
{
    disconnect(m_server,0,0,0);
}

void PluginListen::initPluginListen()
{
    m_server = new QTcpServer(this);
    m_server->setProxy(QNetworkProxy::NoProxy);
    if (!m_server->listen(QHostAddress::Any, 46140)) {
        qDebug()<< __FUNCTION__ << m_server->errorString();
    }
    qDebug()<<__FUNCTION__<<"success";
    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnect()));
}

int PluginListen::initLoginListen()
{
    m_server = new QTcpServer(this);
    int port = 0;
    for (int i = 46140; port < 46155; i++) {
        bool ok = m_server->listen(QHostAddress::Any, i);
        if (ok) {
            port = i;
            break;
        }
    }

    qDebug()<< __FUNCTION__ << port;
    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnect()));
    return port;
}

void PluginListen::savePlugins()
{
    if (USERINFO->isOS())
        return;

    QString spf = USERINFO->allUserPath() + "/SPCache";
    RegEdit::setUS(USERINFO->clientSet(), "SPCache", spf);

    NET->xrget("/bs/system/softwareAndPlugins", [=](FuncBody f){
        QJsonObject obj = f.j;
        int status = obj.take("status").toInt();
        obj.take("detail");
        if (18500000 == status) {
            if (!obj.isEmpty()) {
                //将unity的信息拉取
                {
                    QJsonArray arr = obj.value("rows").toArray();
                    QJsonArray max;
                    QJsonArray Max;
                    QJsonArray sketch;
                    m_unityVersion.clear();
                    for (int i = 0; i < arr.size(); ++i) {
                        QJsonObject obj = arr.at(i).toObject();
                        QString max_soft = obj["software"].toString();
                        if(max_soft.contains("Max"))
                        {
                            Max.append(obj);
                        }
                        QString soft = obj["software"].toString().toLower();
                        if (soft.contains("max")) {
                            max.append(obj);
                        } else if (soft.contains("unity")) {
                            m_unityVersion << soft;
                        } else if (soft.contains("sketch")) {
                            sketch.append(obj);
                        } else if (soft.contains("vray")) {
                            vrverMap.insert(obj["software"].toString(), 0);
                        }
                    }
                    m_plugin.insert("rows", max);
                    m_Max_plugins.insert("rows", Max);
                    m_sketchVersion.insert("rows", sketch);
                }

                QFile nf(spf);
                if (nf.open(QIODevice::WriteOnly)) {
                    nf.write(JsonUtil::jsonObjToByte(m_plugin));
                    nf.close();
                } else
                    qD nf.errorString();
            }
        }
    }, Session::instance()->mainWid());
}

void PluginListen::saveCGs()
{
    if (USERINFO->isOS())
        return;

    QString cgf = USERINFO->allUserPath() + "/CGCache";
    QString f0 = cgf + "0";
    RegEdit::setUS(USERINFO->clientSet(), "CGCache", cgf);

    QString us = "http://op.xrender.com" + QString("/customer/scheme/allSchemeList?userid=%1").arg(USERINFO->userId());
    NET->post(us, "", [=](FuncBody f){
        QJsonArray arr = f.j["data"].toArray();
        if (arr.isEmpty())
            return;

        QByteArray wb = arr.isEmpty() ? QByteArray() : JsonUtil::jsonArrToByte(arr);

        QFile nf(cgf);
        if (nf.open(QFile::WriteOnly)) {
            nf.write(wb);
            nf.close();
        }
        nf.setFileName(f0);
        if (nf.open(QFile::WriteOnly)) {
            QTextStream out(&nf);
            out.setEncoding(QStringConverter::Latin1);
            out << wb;
            nf.close();
        }
    }, Session::instance()->mainWid());
}

void PluginListen::msgOk(QString tip)
{
    QINVOKE(&Session::instance()->mainWid()->m_msgTool, "msgOk",
            Q_ARG(QString, tip), Q_ARG(QWidget*, Session::instance()->mainWid()));
}

QStringList PluginListen::unityVersion()
{
    return m_unityVersion;
}

QJsonObject PluginListen::getPlugins()
{
    return m_Max_plugins;
}

void PluginListen::setMaxPlugins()
{
    QString spf = USERINFO->allUserPath() + "/SPCache";
    QFile f(spf);
    f.open(QFile::ReadOnly);
    QByteArray b = f.readAll();
    m_Max_plugins = JsonUtil::jsonStrToObj(b);
    f.close();
}

void PluginListen::newConnect()
{
    QTcpServer* server = (QTcpServer*)sender();
    if (server == m_server) {
        qD "插件已连接！";
        QTcpSocket* socket = m_server->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(newData()));
    }

    if (server == m_unityServer) {
        m_unityByte.clear();
        qD "unity已连接";
        QTcpSocket* socket = m_unityServer->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(unityData()));
    }

    if (server == m_sketchServer) {
        m_sketchByte.clear();
        qD "sketchup已连接";
        QTcpSocket* socket = m_sketchServer->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(sketchData()));
    }

    if(server == m_vrscene) {
        m_vrsceneByte.clear();
        qD "vrscene已连接";
        QTcpSocket* socket = m_vrscene->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), this, SLOT(vrsceneData()));
    }
}

void httpResponse(QTcpSocket *socket, QByteArray data) {
    QString http = "HTTP/1.1 200 OK\r\n";
    http += "Server: xgt\r\n";
    http += "Content-Type: application/json\r\n";

    QString origin = socket->property("Origin").toString();
    if (!origin.isEmpty()) {
        http += QString("Access-Control-Allow-Origin: %1\r\n").arg(origin);
        http += "Access-Control-Allow-Methods: POST, GET, OPTIONS, PUT, DELETE\r\n";
        http += "Access-Control-Expose-Headers: *\r\n";
        http += "Access-Control-Max-Age: 172800\r\n";
        http += "Access-Control-Allow-Credentials: true\r\n";
    }

    http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(data.length()));
    socket->write(http.toUtf8());

    socket->write(data);
}

void PluginListen::newData()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    QByteArray sb = socket->readAll();
    QStringList sbL = QString(sb).split("\r\n");

    if (sbL.length() > 0) {
        QString f = sbL.first();
        if (f.contains(" HTTP/")) {
            foreach (QString head, sbL) {
                if (head.startsWith("Origin: ")) {
                    socket->setProperty("Origin", head.remove("Origin: "));
                }
            }

            qD "HTTP:" << f;
            if (f.contains("GET") && f.contains("/user/config")) {
                QJsonObject c = USERINFO->userCfg();
                c.insert("permission", JsonUtil::jsonStrToObj(USERINFO->m_xgtPermission));
                c.insert("userId", USERINFO->userIdL());
                c.insert("userName", USERINFO->userName());
                QByteArray data = JsonUtil::jsonObjToByte(c);

                QString string = QString::fromUtf8(data);
                qDebug()<<"Request Body is:"<<string;


                QString http = "HTTP/1.1 200 OK\r\n";
                http += "Server: xgt\r\n";
                http += "Content-Type: application/json\r\n";
                http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(data.length()));
                socket->write(http.toUtf8());

                socket->write(data);

                if(Session::instance()->mainWid()->isUserVip == 0){ //0表示用户身份未被初始化，进行阻断
                    return;
                }
                QMetaObject::invokeMethod(PluginCorrespond::getInstance(), "makeRequestForMaxToClient");
            }
            if (f.contains("GET") && f.contains("/alluser/config/maxSavePath")) {
                QByteArray data = USERINFO->maxPath().toUtf8();
                httpResponse(socket, data);
            }
            if (f.contains("GET") && f.contains("/qqlogin")) {
                QUrlQuery uq(f);
                QJsonObject obj;
                obj.insert("auth", uq.queryItemValue("auth"));
                obj.insert("userInfo", uq.queryItemValue("userinfo"));
                qDebug()<< "登录对象" << obj;
                httpResponse(socket, "login success");
                emit qqlogin(obj);
            }
            if (f.contains("GET") && f.contains("/render/modes")) {
                FuncBody f = NET->sync(GET, renderModeUrl, "", 15);
                QJsonArray arr = f.j.value("data").toObject().value("data").toArray();
                QJsonObject obj;
                obj.insert("modes", arr);
                httpResponse(socket, JsonUtil::jsonObjToByte(obj));
            }
            if (f.contains("GET") && f.contains("/user/subscribe")) {
                QDesktopServices::openUrl(QUrl(USERINFO->openSubscribe()));
                httpResponse(socket, "");
            }
            if (f.contains("GET") && f.contains("/machineCode")) {   //机器码
                QString machineCode = USERINFO->returnMachineCode();
                QJsonObject obj;
                obj.insert("machineCode", machineCode);
                httpResponse(socket, JsonUtil::jsonObjToByte(obj));
            }

            socket->waitForBytesWritten();
            socket->deleteLater();
            return;
        }
    }


    if (!m_stMap.contains(socket)) {
        int len = 0;
        QByteArray ba = sb.mid(0, 4);
        sb.remove(0, 4);
        memcpy_s(&len, sizeof(int), ba.data(), 4);
        m_stMap.insert(socket, len);
        qD __FUNCTION__ << "insert bytelen" << len;
    }

    qint64 blen = sb.length();
    qD __FUNCTION__ << "bytesAvailable" << blen;
    m_stMap.insert(socket, m_stMap.value(socket) - blen);
    m_stbMap.insert(socket, m_stbMap.value(socket) + sb);

    if (m_stMap.value(socket) == 0) {
        m_stMap.remove(socket);

        QByteArray baMsg = m_stbMap.value(socket);
        m_stbMap.remove(socket);

        QJsonObject obj = JsonUtil::jsonStrToObj(baMsg);
        if (!obj.isEmpty())
            commitOrder(obj);
        else
            qD "PluginListen newData err";

        socket->abort();
        socket->deleteLater();
        socket = NULL;
    }
}

void PluginListen::unityData()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    m_unityByte += socket->readAll();

    QJsonObject obj = JsonUtil::jsonStrToObj(m_unityByte);

    if (!obj.isEmpty()) {
        QString root = obj["ProjectRoot"].toString();
        QJsonArray nfa;
        {
            QJsonArray fa = obj["files"].toArray();
            for (int i = 0; i < fa.size(); ++i) {
                QString path = fa.at(i).toString();
                nfa << root + "/" + path;
            }

        }

        obj.insert("files", nfa);

        obj.insert("loadedScenes", obj["loadedScenes"].toArray());
        obj.insert("name", obj["loadedScenes"].toArray().first().toString().split("/").last());
        obj.insert("software", /*"unity 2017.3.1f1"*/"unity " + obj["unityVerison"].toString());

        {
            QString ver = obj["unityVerison"].toString();
            QStringList vL = ver.split(".");
            if (3 == vL.length()) {
                QString val = QString("%1.%2").arg(vL.at(0)).arg(vL.at(1));
                int i = 0;
                for (i; i < m_unityVersion.length(); ++i) {
                    if (m_unityVersion.at(i).contains(val))
                        break;
                }

                if (m_unityVersion.length() == i) {
                    msgOk(tr("不支持%1").arg(obj["software"].toString()));
                    return;
                }
            }
        }

        qD obj;

        commitUnity(obj);
    }
}

void PluginListen::sketchData()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    QByteArray data = socket->readAll();
    qD __FUNCTION__ << data;

    if (data.startsWith("getPlugin")) {
        socket->write(JsonUtil::jsonObjToByte(m_sketchVersion));
        socket->close();
        qD __FUNCTION__ << m_sketchVersion;
    } else {
        m_sketchByte += data;
        QJsonObject suobj = JsonUtil::jsonStrToObj(m_sketchByte);
        if (!suobj.isEmpty()) {
            m_sketchByte.clear();

            commit(2, suobj);
        }
    }
}

void PluginListen::commit(int type, QJsonObject obj)
{
    //新版vrscene加入的丢失文件，不需要传给服务端
    obj.remove("missingFiles");

    qint64 totalSize = 0;
    qint64 stamp = QDateTime::currentMSecsSinceEpoch();
    QList<db_upfile> uL;
    QStringList fL;
    QString maxAbPath;

    {
        QJsonArray fa = obj.value("files").toArray();
        for (int i = 0; i < fa.size(); ++i) {
            QString path = fa.at(i).toString();
            if ((path.endsWith(".max") || path.endsWith(".skp")) && USERINFO->isMaxPath(path)) {
                fL.prepend(path);
                maxAbPath = QFileInfo(path).absoluteDir().path();
            } else
                fL << path;
        }
    }

    for (int i = 0; i < fL.length(); ++i) {
        QString path = fL.at(i);
        if (path.isEmpty())
            continue;
        QFileInfo fI = QFileInfo(path);
        if (!fI.exists()) {
            QString tip = tr("缺少文件 %1 %2").arg(path).arg(i);
            msgOk(tip);
            return;
        }
		if (fI.isDir())
			continue;
        db_upfile uf;
        uf.id = 0;
        uf.lpath = path;
        uf.hash = "";
        uf.size = fI.size();
        totalSize += uf.size;
        uf.state = TransSet::upwait;
        uf.error = 0;
        uf.modtime = fI.lastModified().toMSecsSinceEpoch();
        uf.pri = stamp * 10000 + i;
        uL << uf;
    }

    obj.insert("fileCount", uL.length());
    obj.insert("totalSize", totalSize);
    obj.insert("createTime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    QString outTempFolder = obj["outTempFolder"].toString();
    QString ResultDir = obj.value("ResultDir").toString();

    QByteArray post = JsonUtil::jsonObjToByte(obj);
    NET->post(Request(USERINFO->bs() + "/bs/mission/generate/cg", post, this, 60), [=](FuncBody f) {
        int status = f.j.value("status").toInt();
        bool isRemain = f.j.contains("submitRemain");
        int remain = f.j.value("submitRemain").toInt();
        QString renderName = f.j.value("renderName").toString();
        if (11000700 == status) {
            QString order = f.j.value("num").toString();
            int farmId = f.j.value("farmId").toInt();
            int orderid = f.j.value("missionId").toInt();
            //用最后一个tmp获取订单号
            db_upfile tmp;
            tmp.order = order;
            tmp.farmid = farmId;
            tmp.orderid = orderid;
            UPDAO->Cfile(tmp, uL);

            if ((0 == type || 2 == type) && !maxAbPath.isEmpty() && !maxAbPath.endsWith("Local")) {
                UPDAO->CsrcPath(order, maxAbPath);
            }

            if (1 == type) {
                DDAO->Cunity(order, outTempFolder);
            } else {
                if (!ResultDir.isEmpty() && RenderSet::pushTo() == RenderSet::Src)
                    DDAO->CResultDir(order, ResultDir);
            }
            emit refreshOrder();
            QTimer::singleShot(3000, this, SIGNAL(refreshOrder()));

            if (isRemain) {
                msgOk(tr("您的%1次数还剩%2次").arg(renderName).arg(remain));
            }
        } else {
            if (11000701 == status || 11000702 == status || 11000705 == status || 11000712 == status)
                msgOk(f.j["detail"].toString());
            else if (11000711 == status) {
                msgOk(tr("任务创建失败，权限不足"));
            } else if (11000715 == status) {
                msgOk(tr("您的%1次数已不足，当前任务创建失败").arg(renderName));
            } else {
                msgOk(tr("任务创建失败"));
            }

            //插件需要拷贝图片，所以全部传完再删
            //删除所有文件，是插件备份的文件 appdata/local下
            for(int i = 0; i < uL.size(); i++)
            {
                QString maxPath = uL[i].lpath;
                if (USERINFO->isMaxPath(maxPath)) {
                    XFunc::veryDel(maxPath);
                }
            }
        }
    });
}

void PluginListen::commitOrder(QJsonObject &obj)
{
    QString tip;
    QJsonArray pa = obj["plugins"].toArray();
    qDebug() << "111111111111111 obj = " << obj;
    QJsonArray fa = obj["files"].toArray();
    if (pa.isEmpty() || fa.isEmpty()) {
        tip = tr("插件参数异常");
    }

    if (!tip.isEmpty()) {
        msgOk(tip);
        return;
    }

    //帮插件过滤掉监控中心已经删掉的插件，如果本地已经缓存最新
    {
        QString soft = obj["software"].toString().toLower();
        soft.replace("2010", "2012");
        soft.replace("2011", "2012");
        //VRay为主渲染器时无需校验CoronaRender渲染器
        QString renderer = obj["renderer"].toString().toLower();
        if (!soft.isEmpty() && !renderer.isEmpty()) {
            QStringList white;
            if (renderer.contains("vray"))
                white << "coronarender";

            bool stop = false;
            QString _name;
            QString _version;
            while (!stop && !pa.isEmpty()) {
                QJsonObject _po = pa.takeAt(0).toObject();
                _name = _po["name"].toString();
                _version = _po["version"].toString();
                if (!white.isEmpty()) {
                    if (white.indexOf(_name.toLower()) != -1)
                        continue;
                }


                QJsonArray ra = m_plugin["rows"].toArray();

                for (int i = 0; !stop && i < ra.size(); ++i) {
                    QJsonObject ro = ra.at(i).toObject();
                    if (ro["software"].toString().toLower() == soft) {
                        QJsonArray ropa = ro["plugins"].toArray();
                        for (int j = 0; !stop && j < ropa.size(); ++j) {
                            QString tmp = JsonUtil::jsonObjToStr(ropa.at(j).toObject());
                            tmp = tmp.toLower();
                            if (tmp.contains(_name.toLower()) && tmp.contains(_version.toLower())) {
                                stop = true;
                            }
                        }
                    }
                }
            }
            if (!stop) {
                QString tip = tr("不支持 %1 插件！").arg(_name + _version);
                msgOk(tip);
                return;
            }
        }
    }

    modifyPlugin(obj);

    //更改sp设置
    db_sp sp = userConfig::inst()->sp(obj.value("software").toString());
    if (sp.valid() && sp.select >= 1) {
        obj.insert("envVars", modifySp(sp, obj.value("envVars").toArray()));

        QJsonObject cus = obj.value("custom").toObject();
        cus.insert("env_vars", modifySp(sp, cus.value("env_vars").toArray()));
        obj.insert("custom", cus);
    }

    commit(0, obj);
}

void PluginListen::commitUnity(QJsonObject &obj)
{
    commit(1, obj);
}

void PluginListen::modifyPlugin(QJsonObject &obj)
{
    if(USERINFO->readUserIni("Set", "CustomProjectEn").toInt() == 0)
    {
        return;
    }

    QJsonObject cfg = EnvSet::readProject();
    if(cfg["environment"].toArray().size() == 0)
    {
        return;
    }
    QJsonArray defaultcfg;
    for(int i = 0; i < cfg["environment"].toArray().size(); i++)
    {
        if(cfg["environment"].toArray().at(i).toObject()["software"].toString() == obj["software"].toString())
        {
            defaultcfg = cfg["environment"].toArray().at(i).toObject()["plugins"].toArray();
            break;
        }
    }

    if(defaultcfg.isEmpty())
    {
        return;
    }

    QJsonArray pa = obj["plugins"].toArray();
    qDebug()<< __FUNCTION__ << "脚本传入插件:" << pa;
    qDebug()<< __FUNCTION__ << "自定义配置插件:" << defaultcfg;

    //以defaultcfg优先；同时排除重名插件
    QJsonArray list = defaultcfg;
    foreach (QJsonValue v, pa) {
        list.append(v);
    }
    QJsonArray newplugins;
    QList<QString> nameL;
    for (int i = 0; i < list.size(); i++) {
        QJsonObject plg = list.at(i).toObject();
        QString name = plg.value("name").toString();
        if (!nameL.contains(name)) {
            newplugins << plg;
            nameL << name;
        }
    }

    for(int i = 0; i < newplugins.size(); i++)
    {
        QJsonObject info;
        info["name"] = newplugins.at(i).toObject()["name"].toString();
        info["version"] = newplugins.at(i).toObject()["version"].toString();
        info["originalVersion"] = newplugins.at(i).toObject()["version"].toString();
        newplugins[i] = info;
    }

    obj["plugins"] = newplugins;
}

QJsonArray PluginListen::modifySp(db_sp sp, QJsonArray arr)
{
    QJsonArray env;

    for (int i = 0; i < arr.size(); i++) {
        QString s = arr.at(i).toString();
        if (!s.startsWith("3DSMAX_SERVICE_PACK")) {
            env.append(s);
        }
    }

    env.append(QString("3DSMAX_SERVICE_PACK=%1").arg(sp.getSp()));

    return env;
}

void PluginListen::vrsceneData()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    QByteArray sb = socket->readAll();
    qD __FUNCTION__ << sb;

    QList<QByteArray> sbL = sb.split('\r\n');
    if (sbL.length() > 0) {
        QString f = sbL.first();
        if (f.contains(" HTTP/")) {
            qD "HTTP:" << f;

            QByteArray data = sbL.last();
            QByteArray response = "ok";

            if (f.contains("POST")) {
                if (f.contains("/mission/generate")) {
                    QJsonObject obj = JsonUtil::jsonStrToObj(data);
                    if (!obj.isEmpty()) {
                        bool ok = true;

                        //禁止同名相机
                        QStringList camL;
                        QJsonArray arr = obj.value("rows").toArray();
                        for (int i = 0; i < arr.size(); i++) {
                            QJsonObject sub = arr.at(i).toObject();
                            //相机
                            QString cam = sub.value("camera").toString();
                            if (ok && camL.contains(cam)) {
                                msgOk(tr("场景中存在同名相机，请修改相机名称后进行提交\r\n%1").arg(obj.value("sceneFile").toString()));
                                ok = false;
                            }
                            camL << cam;
                        }

                        if (ok) {
                            if (USERINFO->vrmode() == 1) {
                                emit vrscene(obj);
                            } else {
                            }
                        }
                    } else {
                        qD "vrscene post json error";
                    }
                }
            }

            QString http = "HTTP/1.1 200 OK\r\n";
            http += "Server: xgt\r\n";
            http += "Content-Type: application/json\r\n";
            http += QString("Content-Length: %1\r\n\r\n").arg(QString::number(response.length()));
            socket->write(http.toUtf8());

            socket->write(response);
            socket->waitForBytesWritten();
            socket->deleteLater();
            return;
        }
    }
}
