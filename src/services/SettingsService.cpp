#include "services/SettingsService.h"

#include <QCoreApplication>
#include <QSettings>
#include <utility>

/**
 * @details
 * [General]
 * Hotkey=
 * DisplayMode=
 * StarDictDir=
 * TessdataDir=
 * ResultCardOpacity=
 * ResultCardStyle=
 * QueryHistoryLimit=
 */
namespace {
constexpr auto kSettingsGroupGeneral = "general";
constexpr auto kKeyHotkey = "hotkey";
constexpr auto kKeyDisplayMode = "display_mode";
constexpr auto kKeyStarDictDir = "stardict_dir";
constexpr auto kKeyTessdataDir = "tessdata_dir";
constexpr auto kKeyResultCardOpacity = "result_card_opacity";
constexpr auto kKeyResultCardStyle = "result_card_style";
constexpr auto kKeyQueryHistoryLimit = "query_history_limit";
} // namespace

SettingsService::SettingsService(QString organizationName, QString applicationName)
    : organizationName_(std::move(organizationName)),
      applicationName_(std::move(applicationName)) {
}

AppSettings SettingsService::load() const {
    QSettings settings(organizationName_, applicationName_);

    AppSettings result;
    settings.beginGroup(QString::fromLatin1(kSettingsGroupGeneral));

    result.hotkey = settings.value(QString::fromLatin1(kKeyHotkey), result.hotkey).toString().trimmed();
    if (result.hotkey.isEmpty()) {
        result.hotkey = AppSettings{}.hotkey;
    }

    const QString displayModeRaw = settings
                                       .value(QString::fromLatin1(kKeyDisplayMode), displayModeToString(result.displayMode))
                                       .toString();
    result.displayMode = displayModeFromString(displayModeRaw);

    const QString defaultStarDictDir = QCoreApplication::applicationDirPath() + QStringLiteral("/dict");
    result.starDictDir = settings.value(QString::fromLatin1(kKeyStarDictDir), defaultStarDictDir).toString().trimmed();
    if (result.starDictDir.isEmpty()) {
        result.starDictDir = defaultStarDictDir;
    }

    const QString defaultTessdataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/tessdata");
    result.tessdataDir = settings.value(QString::fromLatin1(kKeyTessdataDir), defaultTessdataDir).toString().trimmed();

    result.resultCardOpacityPercent = clampResultCardOpacityPercent(
        settings
            .value(
                QString::fromLatin1(kKeyResultCardOpacity),
                result.resultCardOpacityPercent)
            .toInt());

    const QString styleRaw = settings
                                 .value(
                                     QString::fromLatin1(kKeyResultCardStyle),
                                     resultCardStyleToString(result.resultCardStyle))
                                 .toString();
    result.resultCardStyle = resultCardStyleFromString(styleRaw);

    result.queryHistoryLimit = clampQueryHistoryLimit(
        settings
            .value(
                QString::fromLatin1(kKeyQueryHistoryLimit),
                result.queryHistoryLimit)
            .toInt());

    settings.endGroup();
    return result;
}

void SettingsService::save(const AppSettings& appSettings) const {
    QSettings settings(organizationName_, applicationName_);
    settings.beginGroup(QString::fromLatin1(kSettingsGroupGeneral));

    settings.setValue(QString::fromLatin1(kKeyHotkey), appSettings.hotkey);
    settings.setValue(QString::fromLatin1(kKeyDisplayMode), displayModeToString(appSettings.displayMode));
    settings.setValue(QString::fromLatin1(kKeyStarDictDir), appSettings.starDictDir);
    settings.setValue(QString::fromLatin1(kKeyTessdataDir), appSettings.tessdataDir);
    settings.setValue(
        QString::fromLatin1(kKeyResultCardOpacity),
        clampResultCardOpacityPercent(appSettings.resultCardOpacityPercent));
    settings.setValue(
        QString::fromLatin1(kKeyResultCardStyle),
        resultCardStyleToString(appSettings.resultCardStyle));
    settings.setValue(
        QString::fromLatin1(kKeyQueryHistoryLimit),
        clampQueryHistoryLimit(appSettings.queryHistoryLimit));

    settings.endGroup();
    settings.sync();
}
