#include "settings.h"
#include "qjsonarray.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

Settings::Settings() {}

Settings loadSettingsFromFile(const QString &filePath)
{
    Settings s;

    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray saveData = file.readAll();
        file.close();

        QJsonDocument doc(QJsonDocument::fromJson(saveData));
        QJsonObject json = doc.object();

        if (json.contains("runOnStartup")) {
            s.runOnstartup = json["runOnStartup"].toBool();
        }
        if (json.contains("terminateOnClose")) {
            s.terminateOnClose = json["terminateOnClose"].toBool();
        }
        if (json.contains("notificationHeadsetDisconnected")) {
            s.notificationHeadsetDisconnected = json["notificationHeadsetDisconnected"].toBool();
        }
        if (json.contains("notificationBatteryFull")) {
            s.notificationBatteryFull = json["notificationBatteryFull"].toBool();
        }
        if (json.contains("notificationBatteryLow")) {
            s.notificationBatteryLow = json["notificationBatteryLow"].toBool();
        }
        if (json.contains("audioNotification")) {
            s.audioNotification = json["audioNotification"].toBool();
        }
        if (json.contains("batteryLowThreshold")) {
            s.batteryLowThreshold = json["batteryLowThreshold"].toInt();
        }
        if (json.contains("msecUpdateIntervalTime")) {
            s.msecUpdateIntervalTime = json["msecUpdateIntervalTime"].toInt();
        }
        if (json.contains("styleName")) {
            s.styleName = json["styleName"].toString();
        }
        if (json.contains("commandExe")) {
            s.commandExe = json["commandExe"].toString();
        }
        if (json.contains("commandArgs")) {
            s.commandArgs = json["commandArgs"].toString();
        }
        if (json.contains("msecCommandIntervalTime")) {
            s.msecCommandIntervalTime = json["msecCommandIntervalTime"].toInt();
        }
        if (json.contains("updateChannel")) {
            s.updateChannel = json["updateChannel"].toString();
        }
        if (json.contains("lastSelectedVendorID")) {
            s.lastSelectedVendorID = json["lastSelectedVendorID"].toString();
        }
        if (json.contains("lastSelectedProductID")) {
            s.lastSelectedProductID = json["lastSelectedProductID"].toString();
        }
        if (json.contains("reapplyAtInterval")) {
            s.reapplyAtInterval = json["reapplyAtInterval"].toBool();
        }
        if (json.contains("reapplyConfigInterval")) {
            s.reapplyConfigInterval = json["reapplyConfigInterval"].toInt();
        }
        if (json.contains("applyOnConnect")) {
            s.applyOnConnect = json["applyOnConnect"].toBool();
        }
        if (json.contains("reapplyCapabilities")) {
            QJsonArray arr(json["reapplyCapabilities"].toArray());
            for (int i = 0; i < arr.size(); ++i) {
                s.reapplyCapabilities.append(arr[i].toString());
            }
        }
        qDebug() << "Settings Loaded:\t" << json;
        qDebug();
    }

    return s;
}

void saveSettingstoFile(const Settings &settings, const QString &filePath)
{
    QJsonObject json;
    json["runOnStartup"] = settings.runOnstartup;
    json["terminateOnClose"] = settings.terminateOnClose;
    json["notificationHeadsetDisconnected"] = settings.notificationHeadsetDisconnected;
    json["notificationBatteryFull"] = settings.notificationBatteryFull;
    json["notificationBatteryLow"] = settings.notificationBatteryLow;
    json["audioNotification"] = settings.audioNotification;
    json["batteryLowThreshold"] = settings.batteryLowThreshold;
    json["msecUpdateIntervalTime"] = settings.msecUpdateIntervalTime;
    json["styleName"] = settings.styleName;
    json["commandExe"] = settings.commandExe;
    json["commandArgs"] = settings.commandArgs;
    json["msecCommandIntervalTime"] = settings.msecCommandIntervalTime;
    json["updateChannel"] = settings.updateChannel;
    json["lastSelectedVendorID"] = settings.lastSelectedVendorID;
    json["lastSelectedProductID"] = settings.lastSelectedProductID;
    json["reapplyAtInterval"] = settings.reapplyAtInterval;
    json["reapplyConfigInterval"] = settings.reapplyConfigInterval;
    json["applyOnConnect"] = settings.applyOnConnect;
    QJsonArray arr;
    for (const QString &cap : settings.reapplyCapabilities) {
        arr.append(cap);
    }
    json["reapplyCapabilities"] = arr;

    QJsonDocument doc(json);
    QFile file(filePath);

    qDebug() << "Saving settings:";
    qDebug() << "Destination:\t" << filePath;
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Error:\tCouldn't open save file.");
    }
    file.write(doc.toJson());
    file.close();

    //qDebug() << "Content:\t" << json;
    qDebug();
}
