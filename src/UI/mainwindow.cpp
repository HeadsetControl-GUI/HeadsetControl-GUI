#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "device.h"
#include "dialoginfo.h"
#include "headsetcontrolapi.h"
#include "loaddevicewindow.h"
#include "settingswindow.h"
#include "utils.h"

#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QProcess>
#include <QScreen>
#include <QStyleHints>
#include <QUrl>
#include <QCursor>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , trayIcon(new QSystemTrayIcon(this))
    , trayMenu(new QMenu(this))
    , timerGUI(new QTimer(this))
    , timerCommand(new QTimer(this))
    , timerReapplyConfig(new QTimer(this))
    , API(HeadsetControlAPI(HEADSETCONTROL_DIRECTORY))
{
    qDebug() << "Headsetcontrol";
    qDebug() << "Name:" << API.getName();
    qDebug() << "Version:" << API.getVersion();
    qDebug() << "ApiVersion:" << API.getApiVersion();
    qDebug() << "HidApiVersion:" << API.getHidApiVersion();
    qDebug();
    qDebug() << "Headsetcontrol-GUI";
    qDebug() << "Version" << qApp->applicationVersion();
    qDebug() << "AppPath" << PROGRAM_CONFIG_PATH;
    qDebug() << "ConfigPath" << PROGRAM_CONFIG_PATH;
    qDebug() << "SettingsPath" << PROGRAM_SETTINGS_FILEPATH;
    qDebug();

    QDir().mkpath(PROGRAM_CONFIG_PATH);
    settings = loadSettingsFromFile(PROGRAM_SETTINGS_FILEPATH);
    API.setSelectedDevice(settings.lastSelectedVendorID, settings.lastSelectedProductID);
    defaultStyle = styleSheet();

    setupTrayIcon();
    ui->setupUi(this);
    bindEvents();

    updateIconsTheme();
    updateStyle();

    updateGUI();

    connect(&API, &HeadsetControlAPI::actionSuccesful, this, &::MainWindow::saveDevicesSettings);

    connect(timerGUI, &QTimer::timeout, this, &::MainWindow::updateGUI);
    timerGUI->start(settings.msecUpdateIntervalTime);
    connect(timerCommand, &QTimer::timeout, this, &::MainWindow::sendCommand);
    if(settings.commandExe != ""){
        timerCommand->start(settings.msecCommandIntervalTime);
    }

    connect(timerReapplyConfig, &QTimer::timeout, this, &MainWindow::reapplySettings);
    if (settings.reapplyConfigEnabled) {
        timerReapplyConfig->start(settings.reapplyConfigInterval * 1000);
    }
}

void sobstituteArgs(QStringList &args, const Device* device){
    for(int i=0; i<args.size(); i++){
        args[i].replace("{chatmix}", QString::number(device->chatmix));
    }
}

void MainWindow::sendCommand(){
    API.updateChatMix();
    QProcess *proc = new QProcess();
    QStringList args = QStringList();
    if(settings.commandArgs != ""){
        args = settings.commandArgs.split(" ");
    }

    sobstituteArgs(args, selectedDevice);
    proc->setProgram(settings.commandExe);
    proc->setArguments(args);

    proc->start();
    proc->waitForFinished();
    qDebug() << "Command Output:\t" << proc->readAllStandardOutput();
    qDebug() << "ExitStatus:\t" << proc->exitStatus();
    qDebug() << "ExitCode:\t" << proc->exitCode();
}

MainWindow::~MainWindow()
{
    timerGUI->stop();
    timerCommand->stop();
    timerReapplyConfig->stop();
    delete timerGUI;
    delete timerCommand;
    delete timerReapplyConfig;
    delete trayMenu;
    delete trayIcon;
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ThemeChange:
        updateIconsTheme();
        break;
    case QEvent::WindowStateChange:
        if (windowState() == Qt::WindowMinimized) {
            hide();
        }
        break;
    default:
        break;
    }

    QMainWindow::changeEvent(e);
}

void MainWindow::bindEvents()
{
    // Tool Bar
    connect(ui->actionLoad_Device, &QAction::triggered, this, &MainWindow::selectDevice);
    connect(ui->actionReload_UI, &QAction::triggered, this, &MainWindow::updateGUI);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::editProgramSetting);
    connect(ui->actionCheck_Updates, &QAction::triggered, this, &MainWindow::checkForUpdates);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionCredits, &QAction::triggered, this, &MainWindow::showCredits);

    //Error frames
    connect(ui->openfolderPushButton, &QPushButton::clicked, this, [=]() {
        openFileExplorer(PROGRAM_APP_DIRECTORY);
    });
    connect(ui->downloadHeadsetControlPushButton, &QPushButton::clicked, this, [=]() {
        updateHeadsetControl(settings.updateChannel);
    });
    connect(ui->refreshPushButton, &QPushButton::clicked, this, [=]() {
        loadDevice();
    });
    connect(ui->headsetControlUpdatePushButton, &QPushButton::clicked, this, [=]() {
        updateHeadsetControl(settings.updateChannel);
    });
    connect(ui->headsetControlGuiUpdatePushButton, &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(
            QUrl("https://github.com/HeadsetControl-GUI/HeadsetControl-GUI/releases/latest"));
    });

    // Other Section
    connect(ui->onlightButton, &QPushButton::clicked, this, [=]() {
        API.setLights(true);
    });
    connect(ui->offlightButton, &QPushButton::clicked, this, [=]() {
        API.setLights(false);
    });
    connect(ui->sidetoneSlider, &QSlider::sliderReleased, this, [=]() {
        int sliderValue = ui->sidetoneSlider->value();
        if (sliderValue != 0) selectedDevice->previous_sidetone = sliderValue;
        API.setSidetone(sliderValue);
    });
    connect(ui->voiceOnButton, &QPushButton::clicked, this, [=]() {
        API.setVoicePrompts(true);
    });
    connect(ui->voiceOffButton, &QPushButton::clicked, this, [=]() {
        API.setVoicePrompts(false);
    });
    connect(ui->notification0Button, &QPushButton::clicked, this, [=]() {
        API.playNotificationSound(0);
    });
    connect(ui->notification1Button, &QPushButton::clicked, this, [=]() {
        API.playNotificationSound(1);
    });
    connect(ui->inactivitySlider, &QSlider::sliderReleased, this, [=]() {
        API.setInactiveTime(ui->inactivitySlider->value());
    });

    // Equalizer Section
    connect(ui->equalizerPresetcomboBox,
            &QComboBox::activated,
            this,
            &MainWindow::equalizerPresetChanged);
    connect(ui->equalizerliveupdateCheckBox, &QCheckBox::checkStateChanged, this, [=](int state) {
        equalizerLiveUpdate = (state == Qt::Checked);
    });
    connect(ui->applyEqualizer, &QPushButton::clicked, this, [=]() { applyEqualizer(); });
    connect(ui->volumelimiterOffButton, &QPushButton::clicked, this, [=]() {
        API.setVolumeLimiter(false);
    });
    connect(ui->volumelimiterOnButton, &QPushButton::clicked, this, [=]() {
        API.setVolumeLimiter(true);
    });

    // Microphone Section
    connect(ui->muteledbrightnessSlider, &QSlider::sliderReleased, this, [=]() {
        API.setMuteLedBrightness(ui->muteledbrightnessSlider->value());
    });
    connect(ui->micvolumeSlider, &QSlider::sliderReleased, this, [=]() {
        API.setMicrophoneVolume(ui->micvolumeSlider->value());
    });
    connect(ui->rotateOn, &QPushButton::clicked, this, [=]() {
        API.setRotateToMute(true);
    });
    connect(ui->rotateOff, &QPushButton::clicked, this, [=]() {
        API.setRotateToMute(false);
    });

    // Bluetooth Section
    connect(ui->btwhenonOffButton, &QPushButton::clicked, this, [=]() {
        API.setBluetoothWhenPoweredOn(false);
    });
    connect(ui->btwhenonOnButton, &QPushButton::clicked, this, [=]() {
        API.setBluetoothWhenPoweredOn(true);
    });
    connect(ui->btbothRadioButton, &QRadioButton::clicked, this, [=]() {
        API.setBluetoothCallVolume(0);
    });
    connect(ui->btpcdbRadioButton, &QRadioButton::clicked, this, [=]() {
        API.setBluetoothCallVolume(1);
    });
    connect(ui->btonlyRadioButton, &QRadioButton::clicked, this, [=]() {
        API.setBluetoothCallVolume(2);
    });
}

//Tray Icon Section
void MainWindow::changeTrayIconTo(QString iconName)
{
    trayIconName = iconName;
    trayIcon->setIcon(QIcon::fromTheme(iconName));
}

void MainWindow::setupTrayIcon()
{
    changeTrayIconTo("headphones");
    trayIcon->setToolTip("HeadsetControl");

    trayMenu->addAction(tr("Hide/Show"), this, &MainWindow::toggleWindow);
    ledOn = trayMenu->addAction(tr("Turn Lights On"), &API, [=]() {
        API.setLights(true);
    });
    ledOff = trayMenu->addAction(tr("Turn Lights Off"), &API, [=]() {
        API.setLights(false);
    });
    trayMenu->addAction(tr("Toggle Sidetone"), &API, [=]() {
        int previousSidetone = selectedDevice->previous_sidetone;
        int currentSidetone = selectedDevice->sidetone;

        selectedDevice->previous_sidetone = currentSidetone;

        if (currentSidetone > 0) API.setSidetone(0, true);
        else if (previousSidetone <= 0 && currentSidetone == 0) {
            API.setSidetone(128, true);
            selectedDevice->previous_sidetone = 128;
        }
        else API.setSidetone(previousSidetone, true);

        ui->sidetoneSlider->setSliderPosition(API.getSelectedDevice()->sidetone);
    });

    trayMenu->addAction(tr("Exit"), this, &QApplication::quit);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->connect(trayIcon,
                      SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                      this,
                      SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        toggleWindow();
    }
}

//Theme mode Section
bool MainWindow::isAppDarkMode()
{
    Qt::ColorScheme scheme = qApp->styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark)
        return true;
    return false;
}

void MainWindow::updateIconsTheme()
{
    if (isAppDarkMode()) {
        QIcon::setThemeName("light");
    } else {
        QIcon::setThemeName("dark");
    }
    setWindowIcon(QIcon::fromTheme("headphones"));
    changeTrayIconTo(trayIconName);
}

void MainWindow::updateStyle()
{
    if (settings.styleName != "Default") {
        QString destination = PROGRAM_STYLES_PATH + "/" + settings.styleName;
        QFile file(destination);
        if (file.open(QFile::ReadOnly)) {
            QString styleSheet = QLatin1String(file.readAll());
            setStyleSheet(styleSheet);
        }
    } else {
        setStyleSheet(defaultStyle);
    }
    rescaleAndMoveWindow();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    rescaleAndMoveWindow();
    if (firstShow) {
        checkForUpdates();
        firstShow = false;
    }
}

//Window Position and Size Section
void MainWindow::toggleWindow()
{
    if (isHidden()) {
        show();
    } else {
        hide();
    }
}

void MainWindow::minimizeWindowSize()
{
    resize(0, 0);
    adjustSize();
}

void MainWindow::moveToBottomRight()
{
    QScreen *screen = QGuiApplication::screenAt(trayIcon->geometry().center());
    if (!screen) {
        screen = QGuiApplication::screenAt(QCursor::pos());
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    QRect screenGeometry = screen->availableGeometry();
    move(screenGeometry.right() - width() - 5, screenGeometry.bottom() - height() - 5);
}

void MainWindow::rescaleAndMoveWindow()
{
    minimizeWindowSize();
    moveToBottomRight();
}

void MainWindow::resetGUI()
{
    trayIcon->setIcon(QIcon::fromTheme("headphones"));
    trayIcon->setToolTip("HeadsetControl");
    ledOn->setEnabled(false);
    ledOff->setEnabled(false);

    ui->missingheadsetcontrolFrame->setHidden(false);
    ui->notSupportedFrame->setHidden(false);

    ui->deviceinfoFrame->setHidden(true);
    ui->batteryFrame->setHidden(true);
    ui->batteryPercentage->setText("");

    ui->tabWidget->hide();
    ui->tabWidget->setTabEnabled(3, false);
    ui->tabWidget->setTabEnabled(2, false);
    ui->tabWidget->setTabEnabled(1, false);
    ui->tabWidget->setTabEnabled(0, false);

    ui->lightFrame->setHidden(true);
    ui->voicepromptFrame->setHidden(true);
    ui->notificationFrame->setHidden(true);
    ui->sidetoneFrame->setHidden(true);
    ui->inactivityFrame->setHidden(true);
    ui->chatmixFrame->setHidden(true);
    ui->volumelimiterFrame->setHidden(true);

    ui->equalizerpresetFrame->setHidden(true);
    ui->equalizerFrame->setHidden(true);
    ui->applyEqualizer->setEnabled(false);
    clearEqualizerSliders();

    ui->rotatetomuteFrame->setHidden(true);
    ui->muteledbrightnessFrame->setHidden(true);
    ui->micvolumeFrame->setHidden(true);

    ui->btwhenonFrame->setHidden(true);
    ui->btcallvolumeFrame->setHidden(true);
}

//Utility Section
void MainWindow::sendAppNotification(const QString &title,
                                     const QString &description,
                                     const QString &icon)
{
    trayIcon->showMessage(title, description, QIcon::fromTheme(icon));
}

//Devices Managing Section
void MainWindow::loadDevice()
{
    resetGUI();

    if (firstRun){
        notified = true;
        firstRun = false;
    }


    selectedDevice = API.getSelectedDevice();
    if (selectedDevice == nullptr) {
        API.setSelectedDevice("0", "0");
        selectedDevice = API.getSelectedDevice();
    }

    if (selectedDevice == nullptr) {
        ui->missingheadsetcontrolFrame->setHidden(true);
        rescaleAndMoveWindow();
        return;
    } else {
        QList<Device *> savedDevices = getSavedDevices();
        selectedDevice->updateConfig(savedDevices);
        deleteDevices(savedDevices);
    }

    QSet<Capability> &capabilities = selectedDevice->capabilities;

    ui->notSupportedFrame->setHidden(true);

    qDebug() << "Selected Device";
    qDebug() << "Device:\t" << selectedDevice->device;
    qDebug() << "Caps:\t" << selectedDevice->capabilities;

    // Info section
    ui->deviceinfovalueLabel->setText(selectedDevice->device + "<br/>" + selectedDevice->vendor
                                      + "<br/>" + selectedDevice->product);
    ui->deviceinfoFrame->setHidden(false);
    if (capabilities.contains(Capability::BATTERY_STATUS)) {
        ui->batteryFrame->setHidden(false);
        setBatteryStatus();
        qDebug() << "Battery:\t" << selectedDevice->battery.status
                 << QString::number(selectedDevice->battery.level);
    }

    ui->tabWidget->show();
    // Other Section
    if (capabilities.contains(Capability::LIGHTS)) {
        ui->lightFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(0, true);
        ledOn->setEnabled(true);
        ledOff->setEnabled(true);
    }
    if (capabilities.contains(Capability::SIDETONE)) {
        ui->sidetoneFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(0, true);
    }
    if (capabilities.contains(Capability::VOICE_PROMPTS)) {
        ui->voicepromptFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(0, true);
    }
    if (capabilities.contains(Capability::NOTIFICATION_SOUND)) {
        ui->notificationFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(0, true);
    }
    if (capabilities.contains(Capability::INACTIVE_TIME)) {
        ui->inactivityFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(0, true);
    }
    if (capabilities.contains(Capability::CHATMIX_STATUS)) {
        ui->chatmixFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(0, true);
        setChatmixStatus();
        qDebug() << "Chatmix:\t" << QString::number(selectedDevice->chatmix);
    }
    // Equalizer Section
    if (capabilities.contains(Capability::EQUALIZER_PRESET) && !selectedDevice->presets_list.empty()) {
        ui->equalizerpresetFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(1, true);
    }
    if (capabilities.contains(Capability::EQUALIZER) && selectedDevice->equalizer.bands_number > 0) {
        ui->equalizerFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(1, true);
    }
    if (capabilities.contains(Capability::VOLUME_LIMITER)) {
        ui->volumelimiterFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(1, true);
    }
    // Microphone Section
    if (capabilities.contains(Capability::ROTATE_TO_MUTE)) {
        ui->rotatetomuteFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(2, true);
    }
    if (capabilities.contains(Capability::MICROPHONE_MUTE_LED_BRIGHTNESS)) {
        ui->muteledbrightnessFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(2, true);
    }
    if (capabilities.contains(Capability::MICROPHONE_VOLUME)) {
        ui->micvolumeFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(2, true);
    }
    // Bluetooth Section
    if (capabilities.contains(Capability::BT_WHEN_POWERED_ON)) {
        ui->btwhenonFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(3, true);
    }
    if (capabilities.contains(Capability::BT_CALL_VOLUME)) {
        ui->btcallvolumeFrame->setHidden(false);
        ui->tabWidget->setTabEnabled(3, true);
    }

    qDebug();
    loadGUIValues();
    rescaleAndMoveWindow();
}

void MainWindow::loadGUIValues()
{
    if (selectedDevice->lights >= 0) {
        ui->onlightButton->setChecked(selectedDevice->lights);
        ui->offlightButton->setChecked(!selectedDevice->lights);
    }
    if (selectedDevice->sidetone >= 0) {
        ui->sidetoneSlider->setSliderPosition(selectedDevice->sidetone);
    }
    if (selectedDevice->voice_prompts >= 0) {
        ui->voiceOnButton->setChecked(selectedDevice->voice_prompts);
        ui->voiceOffButton->setChecked(!selectedDevice->voice_prompts);
    }
    if (selectedDevice->inactive_time >= 0) {
        ui->inactivitySlider->setSliderPosition(selectedDevice->inactive_time);
    }

    clearEqualizerSliders();
    createEqualizerSliders();

    ui->equalizerPresetcomboBox->clear();
    for (EqualizerPreset &preset : selectedDevice->presets_list) {
        ui->equalizerPresetcomboBox->addItem(preset.name);
    }
    ui->equalizerPresetcomboBox->setCurrentIndex(-1);
    if (selectedDevice->equalizer_preset >= 0) {
        ui->equalizerPresetcomboBox->setCurrentIndex(selectedDevice->equalizer_preset);
    } else if (selectedDevice->equalizer_curve.length() == selectedDevice->equalizer.bands_number) {
        setEqualizerSliders(selectedDevice->equalizer_curve);
    }

    if (selectedDevice->volume_limiter >= 0) {
        ui->volumelimiterOnButton->setChecked(selectedDevice->volume_limiter);
        ui->volumelimiterOffButton->setChecked(!selectedDevice->volume_limiter);
    }

    if (selectedDevice->rotate_to_mute >= 0) {
        ui->rotateOn->setChecked(selectedDevice->rotate_to_mute);
        ui->rotateOff->setChecked(!selectedDevice->rotate_to_mute);
    }
    if (selectedDevice->mic_mute_led_brightness >= 0) {
        ui->muteledbrightnessSlider->setSliderPosition(selectedDevice->mic_mute_led_brightness);
    }
    if (selectedDevice->mic_volume >= 0) {
        ui->micvolumeSlider->setSliderPosition(selectedDevice->mic_volume);
    }

    if (selectedDevice->bt_call_volume >= 0) {
        switch (selectedDevice->bt_call_volume) {
        case 0:
            ui->btbothRadioButton->setChecked(true);
            break;
        case 1:
            ui->btpcdbRadioButton->setChecked(true);
            break;
        case 2:
            ui->btonlyRadioButton->setChecked(true);
            break;
        default:
            break;
        }
    }
    if (selectedDevice->bt_when_powered_on >= 0) {
        ui->btwhenonOnButton->setChecked(selectedDevice->bt_when_powered_on);
        ui->btwhenonOffButton->setChecked(!selectedDevice->bt_when_powered_on);
    }
}

void MainWindow::saveDevicesSettings()
{
    QList<Device *> toSave = getSavedDevices();
    updateDeviceFromSource(toSave, selectedDevice);

    serializeDevices(toSave, DEVICES_SETTINGS_FILEPATH);

    deleteDevices(toSave);
}

QList<Device *> MainWindow::getSavedDevices()
{
    return deserializeDevices(DEVICES_SETTINGS_FILEPATH);
}

bool MainWindow::updateSelectedDevice()
{
    QString lastStatus = "";
    if (selectedDevice != nullptr){
        lastStatus = selectedDevice->battery.status;
    }

    API.updateSelectedDevice();
    selectedDevice = API.getSelectedDevice();
    bool stillTheSame = selectedDevice != nullptr;

    if(selectedDevice != nullptr && lastStatus != selectedDevice->battery.status){
        notified = false;
    }

    setBatteryStatus();
    setChatmixStatus();
    return stillTheSame;
}

//Update GUI Section
void MainWindow::updateGUI()
{
    if (!API.areApiAvailable()) {
        resetGUI();
        ui->notSupportedFrame->setHidden(true);
        ui->headsetControlUpdateFrame->setVisible(false);
        ui->headsetControlUpdateRow->setVisible(false);
        ui->headsetControlGuiUpdateRow->setVisible(false);
        rescaleAndMoveWindow();
        selectedDevice = nullptr;
    } else {
        if (selectedDevice == nullptr || !updateSelectedDevice()) {
            loadDevice();
        }
        ui->headsetControlUpdateFrame->setVisible(needsHeadsetControlUpdate || needsGuiUpdate);
        ui->headsetControlUpdateRow->setVisible(needsHeadsetControlUpdate);
        ui->headsetControlGuiUpdateRow->setVisible(needsGuiUpdate);
    }
}

// Info Section Events
void MainWindow::setBatteryStatus()
{
    if (selectedDevice == nullptr) {
        changeTrayIconTo("headphones");
        return;
    }

    QString status = selectedDevice->battery.status;
    int batteryLevel = selectedDevice->battery.level;
    QString level = QString::number(batteryLevel);

    if (batteryLevel >= 0) {
        ui->batteryProgressBar->show();
        ui->batteryProgressBar->setValue(batteryLevel);
    } else {
        ui->batteryProgressBar->hide();
    }

    if (status == "BATTERY_UNAVAILABLE") {
        ui->batteryPercentage->setText(tr("Headset Off"));
        trayIcon->setToolTip(tr("HeadsetControl \r\nHeadset Off"));
        changeTrayIconTo("headphones");
        if (settings.notificationHeadsetDisconnected && !notified) {
            sendAppNotification(tr("Headset Disconnected"),
                                tr("Your headset is disconnected or has been turned off"),
                                "headphones");
            if (settings.audioNotification) {
                API.playNotificationSound(0);
            }
            notified = true;
        }
    } else {
        if (settings.applyOnConnect && (ui->batteryPercentage->text() == tr("Headset Off") || ui->batteryPercentage->text().isEmpty())) {
            reapplySettings();
        }

        if (status == "BATTERY_CHARGING" && batteryLevel >= 0) {
            ui->batteryPercentage->setText(level + tr("% - Charging"));
            trayIcon->setToolTip(tr("HeadsetControl \r\nBattery: Charging - ") + level + "%");
            if (batteryLevel >= 100) {
                changeTrayIconTo("battery-fully-charged");
                if (settings.audioNotification) {
                    API.playNotificationSound(1);
                }
                if (settings.notificationBatteryFull && !notified) {
                    sendAppNotification(tr("Battery Charged!"),
                                        tr("The battery has been charged to 100%"),
                                        "battery-level-full");
                    notified = true;
                }
            } else {
                changeTrayIconTo("battery-charging");
            }
        } else if (status == "BATTERY_AVAILABLE" && batteryLevel >= 0) {
            ui->batteryPercentage->setText(level + tr("% - Descharging"));
            trayIcon->setToolTip(tr("HeadsetControl \r\nBattery: ") + level + "%");
            if (batteryLevel > 75) {
                changeTrayIconTo("battery-level-full");
                notified = false;
            } else if (batteryLevel > settings.batteryLowThreshold) {
                changeTrayIconTo("battery-medium");
                notified = false;
            } else {
                changeTrayIconTo("battery-low");
                if (settings.audioNotification) {
                    API.playNotificationSound(0);
                }
                if (settings.notificationBatteryLow && !notified) {
                    sendAppNotification(tr("Battery Alert!"),
                                        tr("The battery of your headset is running low"),
                                        "battery-low");
                    notified = true;
                }
            }
        } else {
            ui->batteryPercentage->setText(tr("Error getting battery info: ") + level);
            trayIcon->setToolTip(tr("HeadsetControl \r\nBattery: Error ") + level);
            changeTrayIconTo("battery-error");
            if (settings.notificationHeadsetDisconnected && !notified) {
                sendAppNotification(tr("Headset Error"),
                                    tr("Error getting battery info, headset might be turned off"),
                                    "battery-error");
                if (settings.audioNotification) {
                    API.playNotificationSound(0);
                }
                notified = true;
            }
        }
    }
}

void MainWindow::setChatmixStatus()
{
    QString chatmixStatus = tr("None");

    if (selectedDevice == nullptr) {
        ui->chatmixvalueLabel->setText(chatmixStatus);
        return;
    }

    int chatmix = selectedDevice->chatmix;
    QString chatmixValue = QString::number(chatmix);
    if (chatmix < 64)
        chatmixStatus = tr("Game");
    else if (chatmix > 64)
        chatmixStatus = tr("Chat");
    else chatmixStatus = tr("Neutral");

    ui->chatmixvalueLabel->setText(chatmixValue);
    ui->chatmixstatusLabel->setText(chatmixStatus);
}

// Equalizer Section Events
void MainWindow::equalizerPresetChanged()
{
    int index = ui->equalizerPresetcomboBox->currentIndex();
    setEqualizerSliders(selectedDevice->presets_list.value(index).values);
    API.setEqualizerPreset(index);
}

void MainWindow::applyEqualizer(
    bool saveToFile)
{
    ui->equalizerPresetcomboBox->setCurrentIndex(-1);
    QList<double> values;
    for (QSlider *slider : std::as_const(equalizerSliders)) {
        values.append(slider->value() * selectedDevice->equalizer.band_step);
    }
    API.setEqualizer(values, saveToFile);
}

//Equalizer Slidesrs Section
void MainWindow::createEqualizerSliders()
{
    QHBoxLayout *layout = ui->equalizerLayout;
    int &bands_number = selectedDevice->equalizer.bands_number;
    if (bands_number > 0) {
        for (int i = 0; i < bands_number; ++i) {
            QSlider *s = new QSlider(Qt::Vertical);
            s->setMaximum(selectedDevice->equalizer.band_max / selectedDevice->equalizer.band_step);
            s->setMinimum(selectedDevice->equalizer.band_min / selectedDevice->equalizer.band_step);
            s->setSingleStep(1);
            s->setTickInterval(1 / selectedDevice->equalizer.band_step);
            if (selectedDevice->equalizer_curve.size() == bands_number) {
                s->setValue(selectedDevice->equalizer_curve.value(i));
            } else {
                s->setValue(selectedDevice->equalizer.band_baseline);
            }

            equalizerSliders.append(s);
            layout->addWidget(s);
            connect(s, &QAbstractSlider::sliderReleased, this, [=]() {
                if (equalizerLiveUpdate)
                    applyEqualizer(false);
            });
        }
        ui->applyEqualizer->setEnabled(true);
    }
}

void MainWindow::clearEqualizerSliders()
{
    QHBoxLayout *layout = ui->equalizerLayout;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->disconnect();
            widget->deleteLater();
        }
        delete item;
    }
    equalizerSliders.clear();
}

void MainWindow::setEqualizerSliders(
    double value)
{
    for (QSlider *slider : std::as_const(equalizerSliders)) {
        slider->setValue(value / selectedDevice->equalizer.band_step);
    }
}

void MainWindow::setEqualizerSliders(QList<double> values)
{
    int i = 0;
    if (values.length() == selectedDevice->equalizer.bands_number) {
        for (QSlider *slider : std::as_const(equalizerSliders)) {
            slider->setValue((int) (values[i++] / selectedDevice->equalizer.band_step));
        }
    } else {
        setEqualizerSliders(0);
        qDebug() << "ERROR: Bad Equalizer Preset";
    }
}

void MainWindow::reapplySettings()
{
    if (selectedDevice == nullptr || (!settings.reapplyConfigEnabled && !settings.applyOnConnect) || settings.reapplyCapabilities.empty())
        return;

    qDebug() << "Reapplying settings...";
    for (const QString &cap : settings.reapplyCapabilities) {
        if (cap == "lights" && selectedDevice->lights != -1) {
            API.setLights(selectedDevice->lights, false);
        } else if (cap == "sidetone" && selectedDevice->sidetone != -1) {
            API.setSidetone(selectedDevice->sidetone, false);
        } else if (cap == "voice_prompts" && selectedDevice->voice_prompts != -1) {
            API.setVoicePrompts(selectedDevice->voice_prompts, false);
        } else if (cap == "inactive_time" && selectedDevice->inactive_time != -1) {
            API.setInactiveTime(selectedDevice->inactive_time, false);
        } else if (cap == "volume_limiter" && selectedDevice->volume_limiter != -1) {
            API.setVolumeLimiter(selectedDevice->volume_limiter, false);
        } else if (cap == "equalizer" && selectedDevice->equalizer_preset != -1) {
            API.setEqualizerPreset(selectedDevice->equalizer_preset, false);
        } else if (cap == "rotate_to_mute" && selectedDevice->rotate_to_mute != -1) {
            API.setRotateToMute(selectedDevice->rotate_to_mute, false);
        } else if (cap == "mic_mute_led" && selectedDevice->mic_mute_led_brightness != -1) {
            API.setMuteLedBrightness(selectedDevice->mic_mute_led_brightness, false);
        } else if (cap == "mic_volume" && selectedDevice->mic_volume != -1) {
            API.setMicrophoneVolume(selectedDevice->mic_volume, false);
        } else if (cap == "bt_power" && selectedDevice->bt_when_powered_on != -1) {
            API.setBluetoothWhenPoweredOn(selectedDevice->bt_when_powered_on, false);
        } else if (cap == "bt_volume" && selectedDevice->bt_call_volume != -1) {
            API.setBluetoothCallVolume(selectedDevice->bt_call_volume, false);
        }
    }
}

// Tool Bar Events
void MainWindow::selectDevice()
{
    QList<Device *> connectedDevices = API.getConnectedDevices();

    LoaddeviceWindow *loadDevWindow = new LoaddeviceWindow(connectedDevices, this);
    timerGUI->stop();
    if (loadDevWindow->exec() == QDialog::Accepted) {
        int index = loadDevWindow->getDeviceIndex();
        if (index >= 0 && index < connectedDevices.length()) {
            Device* d=connectedDevices[index];
            API.setSelectedDevice(d->id_vendor, d->id_product);
            loadDevice();
            settings.lastSelectedVendorID = selectedDevice->id_vendor;
            settings.lastSelectedProductID = selectedDevice->id_product;
            saveSettingstoFile(settings, PROGRAM_SETTINGS_FILEPATH);
        }
    }
    deleteDevices(connectedDevices);
    delete(loadDevWindow);
    timerGUI->start();
}

void MainWindow::editProgramSetting()
{
    SettingsWindow *settingsW = new SettingsWindow(settings, this);
    connect(settingsW, &SettingsWindow::requestUpdate, this, &MainWindow::updateHeadsetControl);
    if (settingsW->exec() == QDialog::Accepted) {
        settings = settingsW->getSettings();
        saveSettingstoFile(settings, PROGRAM_SETTINGS_FILEPATH);
        timerGUI->setInterval(settings.msecUpdateIntervalTime);
        if(settings.commandExe != ""){
            timerCommand->stop();
            timerCommand->start(settings.msecCommandIntervalTime);
        }
        
        timerReapplyConfig->stop();
        if (settings.reapplyConfigEnabled) {
            timerReapplyConfig->start(settings.reapplyConfigInterval * 1000);
        }
        
        updateStyle();
    }
    delete (settingsW);
}

void MainWindow::checkForUpdates()
{
    if (!API.areApiAvailable()) {
        needsHeadsetControlUpdate = false;
        needsGuiUpdate = false;
        return;
    }

    const QString &hcVersion = API.getVersion();
    const QString &guiVersion = qApp->applicationVersion();
    QString v1;
    if (hcVersion.startsWith("continuous")) {
        v1 = getContinuousGitHubReleaseVersion("Sapd", "HeadsetControl");
    } else {
        v1 = getLatestGitHubReleaseVersion("Sapd", "HeadsetControl");
    }
    QString v2 = getLatestGitHubReleaseVersion("HeadsetControl-GUI", "HeadsetControl-GUI");

    QString hcCleanVersion = hcVersion;
    if (hcCleanVersion.endsWith("-modified")) {
        hcCleanVersion.chop(9);
    }

    needsHeadsetControlUpdate = (!(v1 == "") && v1 != hcCleanVersion && hcCleanVersion != "continuous");
    needsGuiUpdate = (!(v2 == "") && v2 != guiVersion);

    if (needsHeadsetControlUpdate) {
        ui->headsetControlUpdateLabel->setText(
            tr("HeadsetControl update available: ") + v1);
    }
    if (needsGuiUpdate) {
        ui->headsetControlGuiUpdateLabel->setText(
            tr("HeadsetControl-GUI update available: ") + v2);
    }
}

void MainWindow::showAbout()
{
    DialogInfo *dialogWindow = new DialogInfo(this);
    dialogWindow->setTitle(tr("About this program"));
    QString text = tr("You can find HeadsetControl-GUI source code on "
                      "<a href='https://github.com/LeoKlaus/HeadsetControl-GUI'>GitHub</a>.<br/>"
                      "Made by:<br/>"
                      " - <a href='https://github.com/LeoKlaus'>LeoKlaus</a><br/>"
                      " - <a href='https://github.com/nicola02nb'>nicola02nb</a><br/>"
                      "Version: ")
                   + qApp->applicationVersion();
    dialogWindow->setLabel(text);

    dialogWindow->exec();

    delete (dialogWindow);
}

void MainWindow::showCredits()
{
    DialogInfo *dialogWindow = new DialogInfo(this);
    dialogWindow->setTitle(tr("Credits"));
    QString text = tr("Big shout-out to:<br/>"
                      " - <a href='https://github.com/Sapd'>Sapd</a> for <a "
                      "href='https://github.com/Sapd/HeadsetControl'>HeadsetCoontrol");
    dialogWindow->setLabel(text);

    dialogWindow->exec();

    delete (dialogWindow);
}

#include <QMessageBox>
void MainWindow::updateHeadsetControl(QString channel)
{
    QString dirPath = QCoreApplication::applicationDirPath();
    
    QMessageBox::information(this, "Updating", "Downloading and unpacking HeadsetControl " + channel + ". GUI might freeze.", QMessageBox::Ok);
    
    if (downloadAndExtractHeadsetControl(channel, dirPath)) {
        QMessageBox::information(this, "Success", "HeadsetControl updated successfully!");
        timerGUI->stop();
        updateGUI();
        timerGUI->start();
        checkForUpdates();
    } else {
        QMessageBox::warning(this, "Error", "Failed to update HeadsetControl.");
    }
}
