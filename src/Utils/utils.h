#ifndef UTILS_H
#define UTILS_H

#include <QString>

QString getLatestGitHubReleaseVersion(const QString &owner, const QString &repo);
QString getContinuousGitHubReleaseVersion(const QString &owner, const QString &repo);

bool fileExists(const QString &filepath);

bool downloadAndExtractHeadsetControl(const QString &channel, const QString &destDir);

bool openFileExplorer(const QString &path);

bool setOSRunOnStartup(bool enable);

#endif // UTILS_H
