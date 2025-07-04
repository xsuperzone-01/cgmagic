#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#define qD qDebug()<<
class JsonUtil
{
public:
    JsonUtil();

    static QByteArray jsonObjToByte(const QJsonObject& obj);
    static QString jsonObjToStr(const QJsonObject& obj);
    static QString jsonArrToStr(const QJsonArray& arr);
    static QByteArray jsonArrToByte(const QJsonArray& arr);
    static QJsonObject jsonStrToObj(const QString& str);
    static QJsonArray jsonStrToArr(const QString& str);
};

#endif // JSONUTIL_H
