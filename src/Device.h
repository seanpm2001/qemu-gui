#ifndef DEVICE_H
#define DEVICE_H

#include <QWidget>
#include <QtWidgets>

class Device : public QObject
{
    Q_OBJECT
public:
    Device() {}
    Device(const QString &n, Device *parent = 0);

    typedef QList<Device *> Devices;

    void addDevice(Device *dev);
    const Devices &getDevices() const { return devices; }
    QString getDescription() const;
    QString getCommandLine();

    void save(QXmlStreamWriter &xml) const;
    void read(QXmlStreamReader &xml);

    virtual QString getDeviceTypeName() const { return "Device"; }
    virtual QWidget *getEditorForm() { return NULL; }

protected:
    virtual void saveParameters(QXmlStreamWriter &xml) const {}
    virtual void readParameters(QXmlStreamReader &xml) {}
    virtual QString getCommandLineOption() { return ""; }

private:
    QString name;
    Devices devices;
};

#endif // DEVICE_H
