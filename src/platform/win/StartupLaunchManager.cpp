#include "platform/win/StartupLaunchManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include <utility>

namespace {
constexpr auto kRunRegistryPath = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
}

StartupLaunchManager::StartupLaunchManager(QString runValueName)
    : runValueName_(std::move(runValueName)) {
}

bool StartupLaunchManager::setLaunchOnStartupEnabled(const bool enabled, QString* errorMessage) const {
#ifndef Q_OS_WIN
    if (enabled && errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Launch on startup is only supported on Windows in V1.");
    }
    return !enabled;
#else
    QSettings runSettings(QString::fromLatin1(kRunRegistryPath), QSettings::NativeFormat);
    if (enabled) {
        const QString executablePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).trimmed();
        if (executablePath.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to resolve application path for startup registration.");
            }
            return false;
        }
        runSettings.setValue(runValueName_, QStringLiteral("\"%1\"").arg(executablePath));
    } else {
        runSettings.remove(runValueName_);
    }

    runSettings.sync();
    if (runSettings.status() != QSettings::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to update startup registration in Windows registry.");
        }
        return false;
    }

    return true;
#endif
}
