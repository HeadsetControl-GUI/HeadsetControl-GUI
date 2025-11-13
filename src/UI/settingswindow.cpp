#include "settingswindow.h"
#include "ui_settingswindow.h"

#include "utils.h"

#include <QFileDialog>
#include <QStyleHints>

SettingsWindow::SettingsWindow(const Settings &programSettings, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::settingswindow)
{
    setModal(true);
    ui->setupUi(this);

    connect(ui->runonstartupCheckBox, &QCheckBox::clicked, this, &SettingsWindow::setRunOnStartup);
    connect(ui->selectstyleComboBox,
            &QComboBox::currentTextChanged,
            this,
            [this](const QString &text) {
                this->ui->removestylePushButton->setEnabled(text != "Default");
            });
    connect(ui->loadstylePushButton, &QPushButton::clicked, this, &SettingsWindow::saveStyle);
    connect(ui->removestylePushButton, &QPushButton::clicked, this, &SettingsWindow::removeStyle);

    ui->runonstartupCheckBox->setChecked(programSettings.runOnstartup);

    ui->headsetdisconnectednotificationCheckBox->setChecked(programSettings.notificationHeadsetDisconnected);
    ui->batteryfullnotificationCheckBox->setChecked(programSettings.notificationBatteryFull);
    ui->batterylownotificationCheckBox->setChecked(programSettings.notificationBatteryLow);
    ui->batterylowtresholdSpinBox->setValue(programSettings.batteryLowThreshold);
    ui->enableaudioNotificationCheckBox->setChecked(programSettings.audioNotification);

    ui->updateintervaltimeDoubleSpinBox->setValue((double) programSettings.msecUpdateIntervalTime
                                                  / 1000);

    loadStyles();
    ui->selectstyleComboBox->setCurrentIndex(
        ui->selectstyleComboBox->findText(programSettings.styleName));

    ui->commandexeLabel->setText(programSettings.commandExe);
    connect(ui->commandexePushButton, &QPushButton::clicked, this, &SettingsWindow::setCommandExe);
    connect(ui->commandclearPushButton, &QPushButton::clicked, this, &SettingsWindow::clearCommandExe);
    ui->commandargumentsValue->setText(programSettings.commandArgs);
    ui->commandintervaltimeDoubleSpinBox->setValue((double) programSettings.msecCommandIntervalTime
                                         / 1000);
}

Settings SettingsWindow::getSettings()
{
    Settings settings;
    settings.runOnstartup = ui->runonstartupCheckBox->isChecked();
    settings.notificationHeadsetDisconnected = ui->headsetdisconnectednotificationCheckBox->isChecked();
    settings.notificationBatteryFull = ui->batteryfullnotificationCheckBox->isChecked();
    settings.notificationBatteryLow = ui->batterylownotificationCheckBox->isChecked();
    settings.batteryLowThreshold = ui->batterylowtresholdSpinBox->value();
    settings.audioNotification = ui->enableaudioNotificationCheckBox->isChecked();
    settings.msecUpdateIntervalTime = ui->updateintervaltimeDoubleSpinBox->value() * 1000;
    settings.styleName = ui->selectstyleComboBox->currentText();
    settings.commandExe = ui->commandexeLabel->text();
    settings.commandArgs = ui->commandargumentsValue->text();
    settings.msecCommandIntervalTime = ui->commandintervaltimeDoubleSpinBox->value() * 1000;

    return settings;
}

void SettingsWindow::setRunOnStartup()
{
    bool enabled = setOSRunOnStartup(ui->runonstartupCheckBox->isChecked());
    ui->runonstartupCheckBox->setChecked(enabled);
}

void SettingsWindow::loadStyles()
{
    QString destination = PROGRAM_STYLES_PATH;
    QDir directory = QDir(destination);
    QStringList list = directory.entryList(QStringList() << "*.qss", QDir::Files);
    ui->selectstyleComboBox->clear();
    ui->selectstyleComboBox->addItem("Default");
    ui->selectstyleComboBox->addItems(list);
}

void SettingsWindow::saveStyle()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter("QStyle (*.qss)");

    if (dialog.exec())
    {
        QStringList fileUrls = dialog.selectedFiles();
        QUrl fileUrl = QUrl(fileUrls[0]);
        if (fileUrl.isValid()) {
            QString source = fileUrl.path().removeFirst();
            QString destination = PROGRAM_STYLES_PATH;
            QDir().mkpath(destination);
            destination += "/" + fileUrl.fileName();
            QFile file(destination);
            if (file.exists()) {
                file.remove();
            }
            QFile::copy(source, destination);

            loadStyles();
        }
    }
}

void SettingsWindow::removeStyle()
{
    QString stylePath = PROGRAM_STYLES_PATH + "/" + ui->selectstyleComboBox->currentText();
    QFile file(stylePath);
    if (file.exists()) {
        file.remove();
    }

    loadStyles();
}

void SettingsWindow::setCommandExe(){
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);

    if (dialog.exec())
    {
        QStringList fileUrls = dialog.selectedFiles();
        QUrl fileUrl = QUrl(fileUrls[0]);
        if (fileUrl.isValid()) {
            QString source = fileUrl.toString();
            ui->commandexeLabel->setText(source);
        }
    }
}

void SettingsWindow::clearCommandExe(){
    ui->commandexeLabel->setText("");
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}
