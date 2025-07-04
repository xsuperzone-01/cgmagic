#include "transset.h"

#include "tool/xfunc.h"
#include "config/userinfo.h"

TransSet::TransSet()
{
}

//只有读取，无需加锁访问
QString TransSet::codeText(int code)
{
    QString str;
    switch (code) {
    case 100:str = tr("测试码"); break;
    //客户端码
    case 10000:str = tr("文件不存在"); break;
    case 10001:str = tr("文件打开失败"); break;
    case 10010:str = tr("连接超时"); break;
    case 10011:str = tr("协议头长度错误"); break;
    case 10012:str = tr("socket写入失败"); break;
    case 10013:str = tr("startPos错误"); break;
    case 10014:str = tr("文件无法写入，请更换盘符"); break;
    case 10015:str = tr("文件访问失败"); break;//类似网络盘操作中突然读取异常的情况
    case 10016:str = tr("获取文件hash失败"); break;
    case 10017:str = tr("压缩失败"); break;
    case 10018:str = tr("hash校验错误"); break;
    case 10019:str = tr("文件被修改，重新上传"); break;
    case 10020:str = tr("磁盘空间不足，请更换盘符"); break;
    case 10021:str = tr("无法获取下载链接"); break;
    //后台码
    //130100 询问秒传
    case 13010002:str = tr("hash值有误"); break;
    case 13010003:str = tr("记录不存在"); break;
    case 13010004:str = tr("秒传询问未知异常"); break;
    case 13010005:str = tr("订单已取消"); break;
    //压缩询问
    case 13010101:str = tr("请求上传未知异常"); break;
    case 13010102:str = tr("请求上传记录不存在"); break;
    case 13010103:str = tr("上传成功"); break;
    case 13010104:str = tr("后台处理中"); break;
    //完成返回
    case 13010201:str = tr("上传失败未知异常"); break;
    case 13010202:str = tr("上传数据大于文件总大小"); break;
    case 13010203:str = tr("hash校验失败"); break;
    case 13010204:str = tr("文件解压失败"); break;
    case 13010205:str = tr("文件移动至hash库失败"); break;

    case 13020301:str = tr("记录不存在"); break;
    case 13020302:str = tr("未知异常"); break;
    case 13020303:str = tr("文件不存在"); break;
    case 13020304:str = tr("续传位置过大"); break;
    case 13020306:str = tr("文件过大，无法下载"); break;

    case 11040001:str = tr("农场未知异常"); break;
    case 11040002:str = tr("农场不存在"); break;
    case 11040003:str = tr("农场不可用"); break;

    //下载链接
    case 11002201:str = tr("未知异常"); break;
    case 11002202:str = tr("FMS异常"); break;
    case 11002203:str = tr("农场异常"); break;
    case 11002204:str = tr("结果文件不存在"); break;
    case 11002205:str = tr("权限不足"); break;
    case 11002206:str = tr("文件过大"); break;

    //socket error
    case 100000:
    case 110001:
        str = tr("连接服务器拒绝"); break;
    case 100001:
    case 110002:
        str = tr("服务器关闭"); break;
    case 100002:
    case 110003:
        str = tr("未找到服务器"); break;
    case 100003:str = tr("SocketAccessError"); break;
    case 100004:str = tr("SocketResourceError"); break;
    case 100005:
    case 110004:
        str = tr("超时"); break;
    case 100006:str = tr("DatagramTooLargeError"); break;
    case 100007:str = tr("NetworkError"); break;
    case 100008:str = tr("AddressInUseError"); break;
    case 100009:str = tr("SocketAddressNotAvailableError"); break;
    case 100010:str = tr("UnsupportedSocketOperationError"); break;
    case 100011:str = tr("UnfinishedSocketOperationError"); break;
    case 100012:str = tr("ProxyAuthenticationRequiredError"); break;
    case 100013:str = tr("SslHandshakeFailedError"); break;
    case 100014:str = tr("ProxyConnectionRefusedError"); break;
    case 100015:str = tr("ProxyConnectionClosedError"); break;
    case 100016:str = tr("ProxyConnectionTimeoutError"); break;
    case 100017:str = tr("ProxyNotFoundError"); break;
    case 100018:str = tr("ProxyProtocolError"); break;
    case 100019:str = tr("OperationError"); break;
    case 100020:str = tr("SslInternalError"); break;
    case 100021:str = tr("SslInvalidUserDataError"); break;
    case 100022:str = tr("TemporaryError"); break;
    case 99999:str = tr("UnknownSocketError"); break;

    default:str = QString::number(code); break;
    }
    str.prepend(QString("%1_").arg(code));
    return str;
}

void TransSet::transaction(QSqlDatabase &db, TransSet::transact type)
{
    if (type == transact::begin) {
        while (!db.transaction())
            QThread::msleep(100);
    }
    if (type == transact::rollback) {
        while (!db.rollback())
            QThread::msleep(100);
    }
    if (type == transact::commit) {
        while (!db.commit())
            QThread::msleep(100);
    }
}

QString TransSet::transState(int state, int error)
{
    FileState fState = (TransSet::FileState)state;
    QString str;
    switch (fState) {
    case TransSet::upwait:str = tr("等待上传"); break;
    case TransSet::upping:str = tr("上传中"); break;
    case TransSet::uppause:str = tr("上传暂停"); break;
    case TransSet::uperror:
    case TransSet::fileuperror:str = tr("上传异常");break;

    case TransSet::downwait:str = tr("等待下载"); break;
    case TransSet::downing:str = tr("下载中"); break;
    case TransSet::downpause:str = tr("下载暂停"); break;
    case TransSet::downerror:
    case TransSet::filedownerror:
         str = tr("下载异常");
         if (error == 10014 || error == 10020)
             str += QString(":%1").arg(codeText(error).split("_").last());
        break;
    default:break;
    }

    return str;
}

QString TransSet::fileMD5(QFile *file, bool &pause)
{
    QString hashCode;

    bool setOpen = false;
    if (!file->isOpen()) {
        file->open(QFile::ReadOnly);
        setOpen = true;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    if (!file->seek(0))
        return hashCode;
    while (!pause && (file->pos() != file->size())){
        QByteArray data;
        data.resize(1024*1024);
        qint64 fr = file->read(data.data(), data.length());
        if (fr == -1) {
            qDebug()<< "hasherr";
            return hashCode;
        }
        data.resize(fr);
        hash.addData(data);
    }
    file->seek(0);
    hashCode = hash.result().toHex();

    if (setOpen)
        file->close();

    return hashCode;
}

QByteArray TransSet::protocol(int sn, int funcCode, int dataType, qint64 dataLen, QByteArray byte)
{
    QByteArray reqByte;
    reqByte.append(XFunc::intToByte(0x11121314))
            .append(XFunc::intToByte(sn))
            .append(USERINFO->accessToken16())
            .append(XFunc::intToByte(funcCode))
            .append(XFunc::intToByte(dataType))//1:json;2:data
            .append(XFunc::qint64ToByte(dataLen))
            .append(byte);
    return reqByte;
}

int TransSet::dataSize(int size, int time)
{
    qDebug()<< QString("size:%1,time:%2").arg(size).arg(time);
    if (time <= 2000)
        size = size * 1.5;
    else
        size = size / 2;

    if (size < 10240)
        size = 10240;
    if (size > 2 * 1024 * 1024)
        size = 2 * 1024 * 1024;

    return size;
}
