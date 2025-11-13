#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "device.h"
#include "headsetcontrolapi.h"
#include "settings.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QSlider>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    const QString PROGRAM_APP_DIRECTORY = qApp->applicationDirPath();
    QString HEADSETCONTROL_DIRECTORY = "";

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    bool firstShow = true;
    bool firstRun = true;
    bool notified = true;

    QString defaultStyle;

    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QString trayIconName = "headphones";
    QMenu *trayMenu;
    QAction *ledOn;
    QAction *ledOff;
    QTimer *timerGUI;
    QTimer *timerCommand;

    void sendCommand();

    Settings settings;

    int n_connected = 0, n_saved = 0;

    HeadsetControlAPI API;
    Device *selectedDevice = nullptr;

    QList<QSlider *> equalizerSliders;
    bool equalizerLiveUpdate = false;

    void bindEvents();

    //Tray Icon Section
    void changeTrayIconTo(QString iconName);
    void setupTrayIcon();

    //Theme mode Section
    bool isAppDarkMode();
    void updateIconsTheme();
    void updateStyle();

    void resetGUI();

    //Window Position and Size Section
    void minimizeWindowSize();
    void moveToBottomRight();
    void rescaleAndMoveWindow();
    void toggleWindow();
    void showEvent(QShowEvent *event);

    //Utility
    void sendAppNotification(const QString &title, const QString &description, const QString &icon);

    //Devices Managing Section
    bool updateSelectedDevice();
    void loadDevice();
    void loadGUIValues();
    QList<Device *> getSavedDevices();

    // Info Section Events
    void setBatteryStatus();
    void setChatmixStatus();

    //Equalizer Slidesrs Section
    void createEqualizerSliders();
    void clearEqualizerSliders();
    void setEqualizerSliders(double value);
    void setEqualizerSliders(QList<double> values);

private slots:
    void changeEvent(QEvent *e);

    //Tray Icon Section
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

    //Devices Managing Section
    void saveDevicesSettings();

    //Update GUI Section
    void updateGUI();

    // Equalizer Section Events
    void equalizerPresetChanged();
    void applyEqualizer(bool saveToFile = true);

    // Tool Bar Events
    void selectDevice();
    void editProgramSetting();
    void checkForUpdates(bool firstStart = false);
    void showAbout();
    void showCredits();
};
#endif // MAINWINDOW_H
