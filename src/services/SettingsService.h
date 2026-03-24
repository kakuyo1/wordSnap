#pragma once

#include <QString>

#include "app/AppTypes.h"

// Encapsulates all persistent settings read/write logic.
class SettingsService final {
public:
    SettingsService(QString organizationName = QStringLiteral("wordSnap"),
                    QString applicationName = QStringLiteral("wordSnapV1"));

    // Loads settings from QSettings with sane defaults.
    AppSettings load() const;

    // Persists settings to QSettings.
    void save(const AppSettings& appSettings) const;

private:
    QString organizationName_;
    QString applicationName_;
};
