#ifndef MACHINEPROFILES_H
#define MACHINEPROFILES_H

#include <QObject>

class MachineProfiles : public QObject
{
    Q_OBJECT
public:
    explicit MachineProfiles(QObject *parent = nullptr);

signals:

public slots:
    void getMachineInformation();   //获取机器配置信息

    void getMachineCode();  //获取机器设备码
};

#endif // MACHINEPROFILES_H
