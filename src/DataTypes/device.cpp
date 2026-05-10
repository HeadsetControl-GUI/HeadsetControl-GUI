#include "device.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

Battery::Battery() {}

Battery::Battery(QString stat, int lev)
{
    status = stat;
    level = lev;
}

Equalizer::Equalizer() {}

Equalizer::Equalizer(int bands, int baseline, double step, int min, int max)
{
    bands_number = bands;
    band_baseline = baseline;
    band_min = min;
    band_step = step;
    band_max = max;
}

Device::Device() {}

Capability parseCapability(const QString& cap) {
    if (cap == "CAP_BATTERY_STATUS") return Capability::BATTERY_STATUS;
    if (cap == "CAP_CHATMIX_STATUS") return Capability::CHATMIX_STATUS;
    if (cap == "CAP_EQUALIZER_PRESET") return Capability::EQUALIZER_PRESET;
    if (cap == "CAP_EQUALIZER") return Capability::EQUALIZER;
    if (cap == "CAP_LIGHTS") return Capability::LIGHTS;
    if (cap == "CAP_SIDETONE") return Capability::SIDETONE;
    if (cap == "CAP_VOICE_PROMPTS") return Capability::VOICE_PROMPTS;
    if (cap == "CAP_NOTIFICATION_SOUND") return Capability::NOTIFICATION_SOUND;
    if (cap == "CAP_INACTIVE_TIME") return Capability::INACTIVE_TIME;
    if (cap == "CAP_VOLUME_LIMITER") return Capability::VOLUME_LIMITER;
    if (cap == "CAP_ROTATE_TO_MUTE") return Capability::ROTATE_TO_MUTE;
    if (cap == "CAP_MICROPHONE_MUTE_LED_BRIGHTNESS") return Capability::MICROPHONE_MUTE_LED_BRIGHTNESS;
    if (cap == "CAP_MICROPHONE_VOLUME") return Capability::MICROPHONE_VOLUME;
    if (cap == "CAP_BT_WHEN_POWERED_ON") return Capability::BT_WHEN_POWERED_ON;
    if (cap == "CAP_BT_CALL_VOLUME") return Capability::BT_CALL_VOLUME;
    return Capability::UNKNOWN;
}

QDebug operator<<(QDebug debug, const Capability &capability)
{
    switch (capability) {
    case Capability::BATTERY_STATUS: debug << "CAP_BATTERY_STATUS"; break;
    case Capability::CHATMIX_STATUS: debug << "CAP_CHATMIX_STATUS"; break;
    case Capability::EQUALIZER_PRESET: debug << "CAP_EQUALIZER_PRESET"; break;
    case Capability::EQUALIZER: debug << "CAP_EQUALIZER"; break;
    case Capability::LIGHTS: debug << "CAP_LIGHTS"; break;
    case Capability::SIDETONE: debug << "CAP_SIDETONE"; break;
    case Capability::VOICE_PROMPTS: debug << "CAP_VOICE_PROMPTS"; break;
    case Capability::NOTIFICATION_SOUND: debug << "CAP_NOTIFICATION_SOUND"; break;
    case Capability::INACTIVE_TIME: debug << "CAP_INACTIVE_TIME"; break;
    case Capability::VOLUME_LIMITER: debug << "CAP_VOLUME_LIMITER"; break;
    case Capability::ROTATE_TO_MUTE: debug << "CAP_ROTATE_TO_MUTE"; break;
    case Capability::MICROPHONE_MUTE_LED_BRIGHTNESS: debug << "CAP_MICROPHONE_MUTE_LED_BRIGHTNESS"; break;
    case Capability::MICROPHONE_VOLUME: debug << "CAP_MICROPHONE_VOLUME"; break;
    case Capability::BT_WHEN_POWERED_ON: debug << "CAP_BT_WHEN_POWERED_ON"; break;
    case Capability::BT_CALL_VOLUME: debug << "CAP_BT_CALL_VOLUME"; break;
    case Capability::UNKNOWN: debug << "CAP_UNKNOWN"; break;
    }
    return debug;
}

Device::Device(const QJsonObject &jsonObj, QString jsonData)
{
    status = jsonObj["status"].toString();

    device = jsonObj["device"].toString();
    vendor = jsonObj["vendor"].toString();
    product = jsonObj["product"].toString();
    id_vendor = jsonObj["id_vendor"].toString();
    id_product = jsonObj["id_product"].toString();

    QJsonArray caps = jsonObj["capabilities"].toArray();
    for (const QJsonValue &value : std::as_const(caps)) {
        capabilities.insert(parseCapability(value.toString()));
    }
    if (capabilities.contains(Capability::BATTERY_STATUS)) {
        QJsonObject jEq = jsonObj["battery"].toObject();
        battery = Battery(jEq["status"].toString(), jEq["level"].toInt());
    }
    if (capabilities.contains(Capability::CHATMIX_STATUS)) {
        chatmix = jsonObj["chatmix"].toInt();
    }

    if (capabilities.contains(Capability::EQUALIZER_PRESET)) {
        if (jsonObj.contains("equalizer_presets") && jsonObj["equalizer_presets"].isObject()) {
            QJsonObject equalizerPresets = jsonObj["equalizer_presets"].toObject();

            // Parse the original JSON string to find the order of keys
            static QRegularExpression re("\"(\\w+)\":\\s*\\[");
            QRegularExpressionMatchIterator i = re.globalMatch(jsonData);
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                QString presetName = match.captured(1);
                if (equalizerPresets.contains(presetName)) {
                    EqualizerPreset preset;
                    preset.name = presetName;

                    QJsonArray valuesArray = equalizerPresets[presetName].toArray();
                    for (const QJsonValue &value : std::as_const(valuesArray)) {
                        preset.values.append(value.toDouble());
                    }

                    presets_list.append(preset);
                }
            }
        }
    }
    if (capabilities.contains(Capability::EQUALIZER)) {
        QJsonObject jEq = jsonObj["equalizer"].toObject();
        if (!jEq.isEmpty()) {
            equalizer = Equalizer(jEq["bands"].toInt(),
                                  jEq["baseline"].toInt(),
                                  jEq["step"].toDouble(),
                                  jEq["min"].toInt(),
                                  jEq["max"].toInt());
            equalizer_curve = QVector<double>(equalizer.bands_number, equalizer.band_baseline);
        }
    }
}

// Helper functions
bool Device::operator!=(const Device &d) const
{
    return this->id_vendor != d.id_vendor || this->id_product != d.id_product;
}

bool Device::operator==(const Device &d) const
{
    return this->id_vendor == d.id_vendor && this->id_product == d.id_product;
}

bool Device::operator==(const Device *d) const
{
    return this->id_vendor == d->id_vendor && this->id_product == d->id_product;
}

void Device::copyConfig(Device* device){
    this->lights = device->lights;
    this->sidetone = device->sidetone;
    this->previous_sidetone = device->previous_sidetone;
    this->voice_prompts = device->voice_prompts;
    this->inactive_time = device->inactive_time;
    this->equalizer_preset = device->equalizer_preset;
    this->equalizer_curve = device->equalizer_curve;
    this->volume_limiter = device->volume_limiter;
    this->rotate_to_mute = device->rotate_to_mute;
    this->mic_mute_led_brightness = device->mic_mute_led_brightness;
    this->mic_volume = device->mic_volume;
    this->bt_when_powered_on = device->bt_when_powered_on;
    this->bt_call_volume = device->bt_call_volume;
}

void Device::updateConfig(const QList<Device *> &list){
    foreach (Device* device, list) {
        if(*this == device)
            this->copyConfig(device);
    }
}

void Device::updateInfo(const Device *new_device)
{
    this->battery = new_device->battery;
    this->chatmix = new_device->chatmix;
}

QJsonObject Device::toJson() const
{
    QJsonObject json;
    json["device"] = device;
    json["vendor"] = vendor;
    json["product"] = product;
    json["id_vendor"] = id_vendor;
    json["id_product"] = id_product;

    json["lights"] = lights;
    json["sidetone"] = sidetone;
    json["previous_sidetone"] = previous_sidetone;
    json["voice_prompts"] = voice_prompts;
    json["inactive_time"] = inactive_time;
    json["equalizer_preset"] = equalizer_preset;
    json["equalizer_curve"] = QJsonArray::fromVariantList(
        QVariantList(equalizer_curve.begin(), equalizer_curve.end()));
    json["volume_limiter"] = volume_limiter;
    json["rotate_to_mute"] = rotate_to_mute;
    json["mic_mute_led_brightness"] = mic_mute_led_brightness;
    json["mic_volume"] = mic_volume;
    json["bt_when_powered_on"] = bt_when_powered_on;
    json["bt_call_volume"] = bt_call_volume;

    return json;
}

Device *Device::fromJson(
    const QJsonObject &json)
{
    Device *newDev = new Device();
    newDev->device = json["device"].toString();
    newDev->vendor = json["vendor"].toString();
    newDev->product = json["product"].toString();
    newDev->id_vendor = json["id_vendor"].toString();
    newDev->id_product = json["id_product"].toString();

    newDev->lights = json["lights"].toInt();
    newDev->sidetone = json["sidetone"].toInt();
    newDev->previous_sidetone = json["previous_sidetone"].toInt();
    newDev->voice_prompts = json["voice_prompts"].toInt();
    newDev->inactive_time = json["inactive_time"].toInt();
    newDev->equalizer_preset = json["equalizer_preset"].toInt();

    QJsonArray curveArray = json["equalizer_curve"].toArray();
    for (const auto &value : std::as_const(curveArray)) {
        newDev->equalizer_curve.append(value.toDouble());
    }

    newDev->volume_limiter = json["volume_limiter"].toInt();
    newDev->rotate_to_mute = json["rotate_to_mute"].toInt();
    newDev->mic_mute_led_brightness = json["mic_mute_led_brightness"].toInt();
    newDev->mic_volume = json["mic_volume"].toInt();
    newDev->bt_when_powered_on = json["bt_when_powered_on"].toInt();
    newDev->bt_call_volume = json["bt_call_volume"].toInt();

    return newDev;
}

void updateDeviceFromSource(QList<Device *> &devicesToUpdate, const Device *sourceDevice)
{
    bool found = false;
    for (Device *toUpdateDevice : devicesToUpdate) {
        if (*toUpdateDevice == sourceDevice) {
            // Update the saved device with connected device's information
            *toUpdateDevice = *sourceDevice;
            found = true;
        }
    };
    if (!found) {
        Device *toAppend = new Device();
        *toAppend = *sourceDevice;
        devicesToUpdate.append(toAppend);
    }
}

void serializeDevices(const QList<Device *> &devices, const QString &filePath)
{
    QJsonArray jsonArray;
    for (const auto *device : devices) {
        jsonArray.append(device->toJson());
    }

    QJsonDocument doc(jsonArray);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Devices Serialized" << jsonArray;
        qDebug();
    }
}

QList<Device *> deserializeDevices(const QString &filePath)
{
    QList<Device *> devices;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray jsonArray = doc.array();

        for (const auto &value : std::as_const(jsonArray)) {
            Device *device = Device::fromJson(value.toObject());
            devices.append(device);
        }

        file.close();
        qDebug() << "Devices Deserialized" << jsonArray;
        qDebug();
    }
    return devices;
}

void deleteDevices(
    QList<Device *> deviceList)
{
    for (Device *device : deviceList) {
        delete device;
    }
    deviceList.clear();
}
