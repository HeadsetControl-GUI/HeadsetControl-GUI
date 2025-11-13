#include "headsetcontrolapi.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>

HeadsetControlAPI::HeadsetControlAPI(const QString& headsetcontrolDirectory)
{
    if (headsetcontrolDirectory == ""){
        this->headsetcontrolFilePath = "headsetcontrol";
#ifdef Q_OS_WIN
        this->headsetcontrolFilePath += ".exe";
#endif
    } else {
        this->headsetcontrolFilePath = headsetcontrolDirectory;
    }

    parseOutput(sendCommand(QStringList()));
    selectedVendorId="0";
    selectedProductId="0";
}

Device* HeadsetControlAPI::getSelectedDevice()
{
    return selectedDevice;
}

bool HeadsetControlAPI::areApiAvailable(){
    QString executableName = this->headsetcontrolFilePath;
    QFileInfo fileInfo(QDir::currentPath(),executableName);
    if (fileInfo.exists())
        return true;
    else {
        QString path = QStandardPaths::findExecutable(executableName);
        return !path.isEmpty();
    }
}

void HeadsetControlAPI::setSelectedDevice(const QString& vendorId, const QString& productId)
{
    if(vendorId == "")
        selectedVendorId = "0";
    else
        selectedVendorId=vendorId;
    if(productId == "")
        selectedProductId = "0";
    else
        selectedProductId=productId;
    selectedDevice = getDevice();
}

void HeadsetControlAPI::updateSelectedDevice(){
    Device* newD = getDevice();
    if (newD == nullptr){
        selectedDevice = nullptr;
    } else {
        selectedDevice->updateInfo(newD);
    }
}

QString HeadsetControlAPI::getName()
{
    return name;
}

QString HeadsetControlAPI::getVersion()
{
    return version;
}

QString HeadsetControlAPI::getApiVersion()
{
    return api_version;
}

QString HeadsetControlAPI::getHidApiVersion()
{
    return hidapi_version;
}

Device *HeadsetControlAPI::getDevice()
{
    QList<Device *> connectedList = getConnectedDevices(selectedVendorId, selectedProductId);
    Device *connectedDevice = nullptr;
    if(selectedVendorId == "0" && selectedProductId == "0"){
        if (connectedList.size() > 0){
            connectedDevice = connectedList.at(0);
        }
    } else {
        foreach(Device *d, connectedList){
            if(d->id_vendor == selectedVendorId && d->id_product == selectedProductId){
                connectedDevice = d;
            }
        }
    }
    Device *device = nullptr;
    if (connectedDevice != nullptr) {
        device = new Device();
        *device = *connectedDevice;
        selectedVendorId = device->id_vendor;
        selectedProductId = device->id_product;
    }
    deleteDevices(connectedList);

    return device;
}

QList<Device *> HeadsetControlAPI::getConnectedDevices(const QString& vendorId, const QString& productId)
{
    QStringList args = QStringList();
    args << "--device" << vendorId+":"+productId;
    QString output = sendCommand(args);
    QJsonObject jsonInfo = parseOutput(output);

    int device_number = jsonInfo["device_count"].toInt();
    qDebug() << "Found" << device_number << "devices:";

    QList<Device *> devices;
    QJsonArray jsonDevices = jsonInfo["devices"].toArray();
    for (auto jsonDevice: jsonDevices) {
        Device *device = new Device(jsonDevice.toObject(), output);
        devices.append(device);
        qDebug() << device->device << "[" << device->id_vendor << ":" << device->id_vendor << "] ";
    }
    qDebug();

    return devices;
}

// HC rleated functions
QString HeadsetControlAPI::sendCommand(const QStringList &args_list)
{
    QProcess *proc = new QProcess();
    QStringList args = QStringList();
    args << "--output" << "JSON";
    //args << "--test-device"; //Uncomment this to enable test device
    args << args_list;

    proc->setProgram(headsetcontrolFilePath);
    proc->setArguments(args);

    proc->start();
    proc->waitForFinished();
    QString output = proc->readAllStandardOutput();
    qDebug() << "Command:\t" << headsetcontrolFilePath;
    qDebug() << "Args:\t" << args;
    qDebug() << "ExitStatus:\t" << proc->exitStatus();
    qDebug() << "Error:\t" << proc->error();
    qDebug() << "ErrorMessage:\t" << proc->errorString();
    qDebug() << "ExitCode:\t" << proc->exitCode();

    //qDebug() << "Output:" << output;
    qDebug();

    delete (proc);

    return output;
}

QJsonObject HeadsetControlAPI::parseOutput(
    const QString &output)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonInfo = jsonDoc.object();

    name = jsonInfo["name"].toString();
    version = jsonInfo["version"].toString();
    api_version = jsonInfo["api_version"].toString();
    hidapi_version = jsonInfo["hidapi_version"].toString();

    return jsonInfo;
}

Action HeadsetControlAPI::sendAction(const QStringList &args_list)
{
    QStringList args;
    args << "--device" << selectedVendorId+":"+selectedProductId << args_list;
    QString output = sendCommand(args);
    QJsonObject jsonInfo = parseOutput(output);
    QJsonArray actions = jsonInfo["actions"].toArray();
    Action action;
    if (!actions.isEmpty()) {
        QJsonObject jaction = actions[0].toObject();

        action.device = jaction["device"].toString();
        action.capability = jaction["capability"].toString();
        action.status = jaction["status"].toString();
        action.error_message = jaction["error_message"].toString();

        action.success = action.status == "success";

        qDebug() << "Device:\t" << action.device;
        qDebug() << "Capability:" << action.capability;
        qDebug() << "Status:\t" << action.status;
        if (!action.success) {
            qDebug() << "Error:\t" << action.error_message;
        }
        qDebug();
    }

    return action;
}


void HeadsetControlAPI::setSidetone(
    int level, bool emitSignal)
{
    QStringList args = QStringList() << "--sidetone" << QString::number(level);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->sidetone = level;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setLights(
    bool enabled, bool emitSignal)
{
    QStringList args = QStringList() << "--light" << QString::number(enabled);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->lights = enabled;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setVoicePrompts(
    bool enabled, bool emitSignal)
{
    QStringList args = QStringList() << "--voice-prompt" << QString::number(enabled);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->voice_prompts = enabled;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setInactiveTime(
    int time, bool emitSignal)
{
    QStringList args = QStringList() << "--inactive-time" << QString::number(time);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->inactive_time = time;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::playNotificationSound(
    int id, bool emitSignal)
{
    QStringList args = QStringList() << "--notificate" << QString::number(id);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->notification_sound = id;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setVolumeLimiter(
    bool enabled, bool emitSignal)
{
    QStringList args = QStringList() << "--volume-limiter" << QString::number(enabled);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->volume_limiter = enabled;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setEqualizer(
    QList<double> equalizerValues, bool emitSignal)
{
    QString equalizer = "";
    for (double value : equalizerValues) {
        equalizer += QString::number(value) + ",";
    }
    equalizer.removeLast();
    QStringList args = QStringList() << "--equalizer" << equalizer;
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->equalizer_curve = equalizerValues;
        selectedDevice->equalizer_preset = -1;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setEqualizerPreset(
    int number, bool emitSignal)
{
    QStringList args = QStringList() << "--equalizer-preset" << QString::number(number);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->equalizer_preset = number;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setRotateToMute(
    bool enabled, bool emitSignal)
{
    QStringList args = QStringList() << "--rotate-to-mute" << QString::number(enabled);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->rotate_to_mute = enabled;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setMuteLedBrightness(
    int brightness, bool emitSignal)
{
    QStringList args = QStringList() << "--microphone-mute-led-brightness" << QString::number(brightness);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->mic_mute_led_brightness = brightness;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setMicrophoneVolume(
    int volume, bool emitSignal)
{
    QStringList args = QStringList() << "--microphone-volume" << QString::number(volume);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->mic_volume = volume;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setBluetoothWhenPoweredOn(
    bool enabled, bool emitSignal)
{
    QStringList args = QStringList() << "--bt-when-powered-on" << QString::number(enabled);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->bt_when_powered_on = enabled;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::setBluetoothCallVolume(
    int option, bool emitSignal)
{
    QStringList args = QStringList() << "--bt-call-volume" << QString::number(option);
    Action a = sendAction(args);
    if (emitSignal && a.success) {
        selectedDevice->bt_call_volume = option;
        emit actionSuccesful();
    }
}

void HeadsetControlAPI::updateChatMix(){
    if(selectedDevice->capabilities.contains("CAP_CHATMIX_STATUS")){
        selectedDevice->chatmix = -1;
        return;
    }
    QStringList args = QStringList() << "--device" << selectedVendorId+":"+selectedProductId << "--chatmix" << "--output" << "STANDARD";
    QString output = sendCommand(args);
    QString trimmed = output.trimmed();
    int idx = trimmed.lastIndexOf("Chatmix: ");
    if (idx == -1) {
        selectedDevice->chatmix = -1;
        return;
    }

    QString after = trimmed.mid(idx + QStringLiteral("Chatmix: ").length()).trimmed();

    selectedDevice->chatmix = after.toInt();
}
