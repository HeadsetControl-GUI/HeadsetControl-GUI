#include "utils.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>

QString getLatestGitHubReleaseVersion(const QString &owner, const QString &repo)
{
    QEventLoop loop;
    QNetworkAccessManager manager;
    QNetworkRequest request(
        QUrl(QString("https://api.github.com/repos/%1/%2/releases/latest").arg(owner, repo)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject jsonObj = doc.object();
        QString latestVersion = jsonObj.value("tag_name").toString();
        reply->deleteLater();
        return latestVersion;
    } else {
        qDebug() << "Error:" << reply->errorString();
        reply->deleteLater();
        return QString("0.0.0");
    }
}

bool fileExists(const QString &filePath)
{
    QFileInfo checkFile(filePath);
    return checkFile.exists();
}

bool openFileExplorer(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        qDebug() << "Path does not exist:" << path;
        return false;
    }

    QUrl url = QUrl::fromLocalFile(dir.absolutePath());
    return QDesktopServices::openUrl(url);
}

bool setOSRunOnStartup(bool enable)
{
    QString appName = QCoreApplication::applicationName();
    QString appPath = QCoreApplication::applicationFilePath();

#ifdef Q_OS_WIN
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
                          + QDir::separator() + "Startup";
    QString linkPath = startupPath + QDir::separator() + appName + ".lnk";
    if (enable) {
        QFile::remove(linkPath);
        // Use Windows Script Host to create a shortcut with arguments
        QString script =
            "Set oWS = WScript.CreateObject(\"WScript.Shell\")\n"
            "sLinkFile = \"" + linkPath.replace("/", "\\") + "\"\n"
            "Set oLink = oWS.CreateShortcut(sLinkFile)\n"
            "oLink.TargetPath = \"" + appPath.replace("/", "\\") + "\"\n"
            "oLink.WorkingDirectory = \"" + QDir::toNativeSeparators(QCoreApplication::applicationDirPath()) + "\"\n"
            "oLink.Arguments = \"--tray\"\n"
            "oLink.Save\n";
        QString vbsPath = QDir::temp().filePath("create_shortcut.vbs");
        QFile vbsFile(vbsPath);
        if (vbsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            vbsFile.write(script.toUtf8());
            vbsFile.close();
            QProcess::execute("wscript", QStringList() << vbsPath);
            vbsFile.remove();
            return QFile::exists(linkPath);
        }
        return false;
    }
    QFile::remove(linkPath);
    return false;

#elif defined(Q_OS_LINUX)
    String appDir = QCoreApplication::applicationDirPath();
    QString configPath = QDir::homePath() + QDir::separator()  + ".config";
    QString autostartPath = configPath + QDir::separator() + "autostart";
    QString desktopFilePath = autostartPath + QDir::separator()  + appName + "-autostart.desktop";

    QFile::remove(desktopFilePath);

    if (enable) {
        QFile::remove(desktopFilePath);
        QDir configDir(configPath);
        QFile desktopFile(desktopFilePath);
        if (!configDir.exists("autostart")) {
            configDir.mkdir("autostart");
        }

        if (desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&desktopFile);
            out << "[Desktop Entry]\n";
            out << "Path=" + appDir + "\n";
            out << "Type=Application\n";
            out << "Hidden=false\n";
            out << "NoDisplay=false\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            out << "Exec=" << appPath << " --tray\n";
            out << "Name=" << appName << "\n";
            out << "Icon=" << appName << "\n";
            out << "Comment=Auto-starts " << appName << " on boot\n";
            desktopFile.close();
            return true;
        }
    }

    return false;
#endif
}
