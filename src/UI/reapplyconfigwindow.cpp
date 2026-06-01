#include "reapplyconfigwindow.h"
#include "ui_reapplyconfigwindow.h"

ReapplyConfigWindow::ReapplyConfigWindow(const Settings &programSettings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReapplyConfigWindow),
    settings(programSettings)
{
    ui->setupUi(this);

    capMap["lights"] = ui->cap_lights;
    capMap["sidetone"] = ui->cap_sidetone;
    capMap["voice_prompts"] = ui->cap_voice_prompts;
    capMap["inactive_time"] = ui->cap_inactive_time;
    capMap["volume_limiter"] = ui->cap_volume_limiter;
    capMap["equalizer"] = ui->cap_equalizer;
    capMap["rotate_to_mute"] = ui->cap_rotate_to_mute;
    capMap["mic_mute_led"] = ui->cap_mic_mute_led;
    capMap["mic_volume"] = ui->cap_mic_volume;
    capMap["bt_power"] = ui->cap_bt_power;
    capMap["bt_volume"] = ui->cap_bt_volume;

    loadSettings();
}

ReapplyConfigWindow::~ReapplyConfigWindow()
{
    delete ui;
}

void ReapplyConfigWindow::loadSettings()
{
    ui->reapplyEnableCheckBox->setChecked(settings.reapplyAtInterval);
    ui->intervalSpinBox->setValue(settings.reapplyConfigInterval);
    ui->connectApplyCheckBox->setChecked(settings.applyOnConnect);

    for (const QString &cap : settings.reapplyCapabilities) {
        if (capMap.contains(cap)) {
            capMap[cap]->setChecked(true);
        }
    }
}

Settings ReapplyConfigWindow::getSettings()
{
    settings.reapplyAtInterval = ui->reapplyEnableCheckBox->isChecked();
    settings.reapplyConfigInterval = ui->intervalSpinBox->value();
    settings.applyOnConnect = ui->connectApplyCheckBox->isChecked();

    settings.reapplyCapabilities.clear();
    QMapIterator<QString, QCheckBox*> i(capMap);
    while (i.hasNext()) {
        i.next();
        if (i.value()->isChecked()) {
            settings.reapplyCapabilities.append(i.key());
        }
    }

    return settings;
}
