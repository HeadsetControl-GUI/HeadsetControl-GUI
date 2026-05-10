#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include "settings.h"

#include <QDialog>

namespace Ui {
class settingswindow;
}

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(const Settings &programSettings, QWidget *parent = nullptr);
    ~SettingsWindow();

    Settings getSettings();

signals:
    void requestUpdate(QString channel);

private:
    Ui::settingswindow *ui;
    Settings tempSettings;

    void setRunOnStartup();
    void loadStyles();
    void saveStyle();
    void removeStyle();
    void setCommandExe();
    void clearCommandExe();
};

#endif // SETTINGSWINDOW_H
