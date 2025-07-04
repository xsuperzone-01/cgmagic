#include "jsonutil.h"

JsonUtil::JsonUtil()
{
}

QByteArray JsonUtil::jsonObjToByte(const QJsonObject &obj)
{
    QJsonDocument document;
    document.setObject(obj);
    return document.toJson(QJsonDocument::Compact);
}

QString JsonUtil::jsonObjToStr(const QJsonObject &obj)
{
    return QString::fromUtf8(jsonObjToByte(obj));
}

QString JsonUtil::jsonArrToStr(const QJsonArray &arr)
{
    return QString::fromUtf8(jsonArrToByte(arr));
}

QByteArray JsonUtil::jsonArrToByte(const QJsonArray &arr)
{
    QJsonDocument document;
    document.setArray(arr);
    const QByteArray& ba = document.toJson(QJsonDocument::Compact);
    return ba;
}

QJsonObject JsonUtil::jsonStrToObj(const QString &str)
{
    const QJsonDocument& doc = QJsonDocument::fromJson(str.toUtf8());
    return doc.object();
}

QJsonArray JsonUtil::jsonStrToArr(const QString &str)
{
    const QJsonDocument& doc = QJsonDocument::fromJson(str.toUtf8());
    return doc.array();
}
