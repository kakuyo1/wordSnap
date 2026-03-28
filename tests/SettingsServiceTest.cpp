#include <QtTest/QtTest>

#include <QSettings>
#include <QUuid>

#include "services/SettingsService.h"

namespace {
QString makeIsolatedName(const QString& prefix) {
    return QStringLiteral("%1_%2")
        .arg(prefix, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

void clearSettingsScope(const QString& organizationName, const QString& applicationName) {
    QSettings settings(organizationName, applicationName);
    settings.clear();
    settings.sync();
}
} // namespace

class SettingsServiceTest : public QObject {
    Q_OBJECT

private slots:
    void loadUsesDefaultLaunchOnStartupWhenMissing();
    void saveAndLoadPersistsLaunchOnStartupEnabled();
    void saveAndLoadPersistsLaunchOnStartupDisabled();
};

void SettingsServiceTest::loadUsesDefaultLaunchOnStartupWhenMissing() {
    const QString organizationName = makeIsolatedName(QStringLiteral("wordSnapTestOrg"));
    const QString applicationName = makeIsolatedName(QStringLiteral("wordSnapTestApp"));
    clearSettingsScope(organizationName, applicationName);

    SettingsService service(organizationName, applicationName);
    const AppSettings loaded = service.load();

    QCOMPARE(loaded.launchOnStartup, false);
    clearSettingsScope(organizationName, applicationName);
}

void SettingsServiceTest::saveAndLoadPersistsLaunchOnStartupEnabled() {
    const QString organizationName = makeIsolatedName(QStringLiteral("wordSnapTestOrg"));
    const QString applicationName = makeIsolatedName(QStringLiteral("wordSnapTestApp"));
    clearSettingsScope(organizationName, applicationName);

    SettingsService service(organizationName, applicationName);
    AppSettings settings = service.load();
    settings.launchOnStartup = true;
    service.save(settings);

    const AppSettings loaded = service.load();
    QCOMPARE(loaded.launchOnStartup, true);
    clearSettingsScope(organizationName, applicationName);
}

void SettingsServiceTest::saveAndLoadPersistsLaunchOnStartupDisabled() {
    const QString organizationName = makeIsolatedName(QStringLiteral("wordSnapTestOrg"));
    const QString applicationName = makeIsolatedName(QStringLiteral("wordSnapTestApp"));
    clearSettingsScope(organizationName, applicationName);

    SettingsService service(organizationName, applicationName);
    AppSettings settings = service.load();
    settings.launchOnStartup = true;
    service.save(settings);
    settings.launchOnStartup = false;
    service.save(settings);

    const AppSettings loaded = service.load();
    QCOMPARE(loaded.launchOnStartup, false);
    clearSettingsScope(organizationName, applicationName);
}

QTEST_APPLESS_MAIN(SettingsServiceTest)

#include "SettingsServiceTest.moc"
