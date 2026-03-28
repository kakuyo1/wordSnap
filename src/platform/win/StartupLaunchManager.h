#pragma once

#include <QString>

class StartupLaunchManager final {
public:
    StartupLaunchManager(QString runValueName = QStringLiteral("wordSnapV1"));

    bool setLaunchOnStartupEnabled(bool enabled, QString* errorMessage = nullptr) const;

private:
    QString runValueName_;
};
