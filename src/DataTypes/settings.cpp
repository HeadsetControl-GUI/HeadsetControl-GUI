#include "settings.h"

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
        if (json.contains("notificationHeadsetDisconnected")) {
            s.batteryLowThreshold = json["notificationHeadsetDisconnected"].toInt();
        }
        if (json.contains("notificationBatteryFull")) {
            s.batteryLowThreshold = json["notificationBatteryFull"].toInt();
        }
        if (json.contains("notificationBatteryLow")) {
            s.batteryLowThreshold = json["notificationBatteryLow"].toInt();
        }
        if (json.contains("audioNotification")) {
            s.batteryLowThreshold = json["audioNotification"].toInt();
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
        if (json.contains("lastSelectedVendorID")) {
            s.lastSelectedVendorID = json["lastSelectedVendorID"].toString();
        }
        if (json.contains("lastSelectedProductID")) {
            s.lastSelectedProductID = json["lastSelectedProductID"].toString();
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
    json["notificationHeadsetDisconnected"] = settings.notificationHeadsetDisconnected;
    json["notificationBatteryFull"] = settings.notificationBatteryFull;
    json["notificationBatteryLow"] = settings.notificationBatteryLow;
    json["audioNotification"] = settings.audioNotification;
    json["batteryLowThreshold"] = settings.batteryLowThreshold;
    json["msecUpdateIntervalTime"] = settings.msecUpdateIntervalTime;
    json["styleName"] = settings.styleName;
    json["lastSelectedVendorID"] = settings.lastSelectedVendorID;
    json["lastSelectedProductID"] = settings.lastSelectedProductID;

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
