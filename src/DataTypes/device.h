#ifndef DEVICE_H
#define DEVICE_H

#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QDebug>

class Battery
{
public:
    Battery();
    Battery(QString stat, int lev);
    QString status = "BATTERY_UNAVAILABLE";
    int level = 0;
};

class EqualizerPreset
{
public:
    QString name;
    QList<double> values;
};

class Equalizer
{
public:
    Equalizer();
    Equalizer(int bands, int baseline, double step, int min, int max);

    int bands_number = 0;
    int band_baseline = 0;
    double band_step = 0;
    int band_min = 0;
    int band_max = 0;
};

enum class Capability
{
    BATTERY_STATUS,
    CHATMIX_STATUS,
    EQUALIZER_PRESET,
    EQUALIZER,
    LIGHTS,
    SIDETONE,
    VOICE_PROMPTS,
    NOTIFICATION_SOUND,
    INACTIVE_TIME,
    VOLUME_LIMITER,
    ROTATE_TO_MUTE,
    MICROPHONE_MUTE_LED_BRIGHTNESS,
    MICROPHONE_VOLUME,
    BT_WHEN_POWERED_ON,
    BT_CALL_VOLUME,
    UNKNOWN
};

QDebug operator<<(QDebug debug, const Capability &capability);

class Device
{
public:
    Device();
    Device(const QJsonObject &jsonObj, QString jsonData);

    // Status
    QString status;

    // Basic info
    QString device;
    QString vendor;
    QString product;
    QString id_vendor;
    QString id_product;
    QSet<Capability> capabilities;

    // Info to get from json and display
    Battery battery;
    int chatmix = 64;
    QList<EqualizerPreset> presets_list;
    Equalizer equalizer;
    bool notification_sound = false;

    // Info to set with gui and to save
    int lights = -1;
    int sidetone = -1;
    int previous_sidetone = -1;
    int voice_prompts = -1;
    int inactive_time = -1;
    int equalizer_preset = -1;
    QList<double> equalizer_curve;
    int volume_limiter = -1;
    int rotate_to_mute = -1;
    int mic_mute_led_brightness = -1;
    int mic_volume = -1;
    int bt_when_powered_on = -1;
    int bt_call_volume = -1;

    bool operator!=(const Device &d) const;
    bool operator==(const Device &d) const;
    bool operator==(const Device *d) const;

    void copyConfig(Device* device);
    void updateConfig(const QList<Device *> &list);

    void updateInfo(const Device *new_device);

    QJsonObject toJson() const;
    static Device *fromJson(const QJsonObject &json);
};

void updateDeviceFromSource(QList<Device *> &devicesToUpdate, const Device *sourceDevice);

void serializeDevices(const QList<Device *> &devices, const QString &filePath);
QList<Device *> deserializeDevices(const QString &filePath);

void deleteDevices(QList<Device *> deviceList);

#endif // DEVICE_H
