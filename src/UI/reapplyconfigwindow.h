#ifndef REAPPLYCONFIGWINDOW_H
#define REAPPLYCONFIGWINDOW_H

#include "settings.h"
#include <QDialog>
#include <QCheckBox>
#include <QMap>

namespace Ui {
class ReapplyConfigWindow;
}

class ReapplyConfigWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ReapplyConfigWindow(const Settings &programSettings, QWidget *parent = nullptr);
    ~ReapplyConfigWindow();

    Settings getSettings();

private:
    Ui::ReapplyConfigWindow *ui;
    Settings settings;
    QMap<QString, QCheckBox*> capMap;

    void loadSettings();
};

#endif // REAPPLYCONFIGWINDOW_H
