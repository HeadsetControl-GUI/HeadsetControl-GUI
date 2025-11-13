#ifndef HEADSETCONTROLAPI_H
#define HEADSETCONTROLAPI_H

#include "device.h"

#include <QFile>
#include <QObject>
#include <QVersionNumber>

class Action
{
public:
    bool success;
    QString capability;
    QString device;
    QString status;
    QString error_message;
};

class HeadsetControlAPI : public QObject
{
    Q_OBJECT
public:
    HeadsetControlAPI(const QString& headsetcontrolDirectory = "");

    bool areApiAvailable();

    QString getName();
    QString getVersion();
    QString getApiVersion();
    QString getHidApiVersion();

    Device* getSelectedDevice();

    void setSelectedDevice(const QString& vendorId, const QString& productId);
    void updateSelectedDevice();

    QList<Device *> getConnectedDevices(const QString& vendorId = "0", const QString& productId = "0");

private:
    QString headsetcontrolFilePath;
    QFile headsetcontrol;

    QString selectedVendorId;
    QString selectedProductId;
    Device *selectedDevice = nullptr;
    QString name;
    QString version;
    QString api_version;
    QString hidapi_version;

    QString sendCommand(const QStringList &args_list);
    Action sendAction(const QStringList &args_list);
    QJsonObject parseOutput(const QString &output);
    Device *getDevice();

public slots:
    void setSidetone(int level, bool emitSignal = true);
    void setLights(bool enabled, bool emitSignal = true);
    void setVoicePrompts(bool enabled, bool emitSignal = true);
    void setInactiveTime(int time, bool emitSignal = true);
    void playNotificationSound(int id, bool emitSignal = true);
    void setVolumeLimiter(bool enabled, bool emitSignal = true);
    void setEqualizer(QList<double> equalizerValues, bool emitSignal = true);
    void setEqualizerPreset(int number, bool emitSignal = true);

    void setRotateToMute(bool enabled, bool emitSignal = true);
    void setMuteLedBrightness(int brightness, bool emitSignal = true);
    void setMicrophoneVolume(int volume, bool emitSignal = true);

    void setBluetoothWhenPoweredOn(bool enabled, bool emitSignal = true);
    void setBluetoothCallVolume(int option, bool emitSignal = true);

    void updateChatMix();

signals:
    void actionSuccesful();
};

#endif // HEADSETCONTROLAPI_H
