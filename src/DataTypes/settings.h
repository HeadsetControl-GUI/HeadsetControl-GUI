#ifndef SETTINGS_H
#define SETTINGS_H

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>

#ifdef QT_DEBUG
const QString PROGRAM_CONFIG_PATH = "./DEBUG-Config";
#else
const QString PROGRAM_CONFIG_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                    + QDir::separator() + "HeadsetControl-GUI";
#endif
const QString PROGRAM_STYLES_PATH = PROGRAM_CONFIG_PATH + QDir::separator() + "styles";
const QString PROGRAM_SETTINGS_FILEPATH = PROGRAM_CONFIG_PATH + QDir::separator() + "settings.json";
const QString DEVICES_SETTINGS_FILEPATH = PROGRAM_CONFIG_PATH + QDir::separator() + "devices.json";

class Settings
{
public:
    Settings();

    bool runOnstartup = false;

    bool notificationHeadsetDisconnected = true;
    bool notificationBatteryFull = true;
    bool notificationBatteryLow = true;
    int batteryLowThreshold = 15;
    bool audioNotification = true;

    int msecUpdateIntervalTime = 30000;

    QString styleName = "Default";

    QString commandExe = "";
    QString commandArgs = "{chatmix}";
    int msecCommandIntervalTime = 1000;

    QString lastSelectedVendorID = "", lastSelectedProductID = "";
};

Settings loadSettingsFromFile(const QString &filePath);
void saveSettingstoFile(const Settings &settings, const QString &filePath);

#endif // SETTINGS_H
