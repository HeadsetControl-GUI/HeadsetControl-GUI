QT += core gui network
greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

CONFIG += c++17

INCLUDEPATH += \
    src/DataTypes \
    src/UI \
    src/Utils

SOURCES += \
    src/UI/settingswindow.cpp \
    src/Utils/headsetcontrolapi.cpp \
    src/main.cpp \
    src/DataTypes/device.cpp \
    src/DataTypes/settings.cpp \
    src/UI/dialoginfo.cpp \
    src/UI/loaddevicewindow.cpp \
    src/UI/mainwindow.cpp \
    src/UI/reapplyconfigwindow.cpp \
    src/Utils/utils.cpp

HEADERS += \
    src/DataTypes/device.h \
    src/DataTypes/settings.h \
    src/UI/dialoginfo.h \
    src/UI/loaddevicewindow.h \
    src/UI/mainwindow.h \
    src/UI/reapplyconfigwindow.h \
    src/UI/settingswindow.h \
    src/Utils/headsetcontrolapi.h \
    src/Utils/utils.h

FORMS += \
    src/UI/dialoginfo.ui \
    src/UI/loaddevicewindow.ui \
    src/UI/mainwindow.ui \
    src/UI/reapplyconfigwindow.ui \
    src/UI/settingswindow.ui

TRANSLATIONS += \
    src/Resources/tr/HeadsetControl_GUI_en.ts \
    src/Resources/tr/HeadsetControl_GUI_it.ts

RESOURCES += \
    src/Resources/icons.qrc

RC_FILE = src/Resources/appicon.rc

DISTFILES += \
    .gitignore

CONFIG += lrelease
QM_FILES_RESOURCE_PREFIX=/translations/tr
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Pass the GUI_VERSION_VALUE from qmake command line to C++ preprocessor
!isEmpty(GUI_VERSION_VALUE) {
    DEFINES += GUI_VERSION_VALUE=\\\"$$GUI_VERSION_VALUE\\\"
}
