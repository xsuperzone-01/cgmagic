#include "machineprofiles.h"
#include "tool/xfunc.h"
#include <QJsonObject>
#include "config/userinfo.h"

MachineProfiles::MachineProfiles(QObject *parent) : QObject(parent)
{

}

//获取机器配置信息
void MachineProfiles::getMachineInformation(){
    MachineInfo info;
    XFunc::SYS(info);
    QString mac = XFunc::MAC();
    QString vol = XFunc::VOL();
    QString sw = QString::number(XFunc::ScreenWidth());
    QString sh = QString::number(XFunc::ScreenHeight());

    QJsonObject obj;
    obj.insert("os", info.OS);
    obj.insert("cpu", info.CPU);
    obj.insert("mac", mac);
    obj.insert("disk", vol);
    obj.insert("mainboard", info.MotherBoard);
    obj.insert("graphics", info.DisplayCard);
    obj.insert("memory", info.Memory);
    obj.insert("screenW", sw);
    obj.insert("screenH", sh);
    obj.insert("maxVersions", "");

    UserInfo::instance()->machineInformation = obj;
}

void MachineProfiles::getMachineCode(){
    QString md5_machine;
    MachineInfo info;

    XFunc::SYS(info);
    QString mac = XFunc::MAC(true);
    QString cpu = info.CPU;
    QString mainboard = info.MotherBoard;
    QString combine = QString("%1%2%3").arg(cpu).arg(mac).arg(mainboard);
    md5_machine = XFunc::Md5(combine).toUpper().mid(8, 16);

    USERINFO->setMachineCode(md5_machine);
}
