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
 * ResultCardDurationMs=
 * ResultCardStyle=
 * QueryHistoryLimit=
 * AiAssistEnabled=
 * AiApiKey=
 * AiBaseUrl=
 * AiModel=
 * AiTimeoutMs=
 */
namespace {
constexpr auto kSettingsGroupGeneral = "general";
constexpr auto kKeyHotkey = "hotkey";
constexpr auto kKeyDisplayMode = "display_mode";
constexpr auto kKeyStarDictDir = "stardict_dir";
constexpr auto kKeyTessdataDir = "tessdata_dir";
constexpr auto kKeyResultCardOpacity = "result_card_opacity";
constexpr auto kKeyResultCardDurationMs = "result_card_duration_ms";
constexpr auto kKeyResultCardStyle = "result_card_style";
constexpr auto kKeyQueryHistoryLimit = "query_history_limit";
constexpr auto kKeyAiAssistEnabled = "ai_assist_enabled";
constexpr auto kKeyAiApiKey = "ai_api_key";
constexpr auto kKeyAiBaseUrl = "ai_base_url";
constexpr auto kKeyAiModel = "ai_model";
constexpr auto kKeyAiTimeoutMs = "ai_timeout_ms";
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

    result.resultCardDurationMs = clampResultCardDurationMs(
        settings
            .value(
                QString::fromLatin1(kKeyResultCardDurationMs),
                result.resultCardDurationMs)
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

    result.aiAssistEnabled = settings
                                 .value(
                                     QString::fromLatin1(kKeyAiAssistEnabled),
                                     result.aiAssistEnabled)
                                 .toBool();
    result.aiApiKey = settings
                          .value(
                              QString::fromLatin1(kKeyAiApiKey),
                              result.aiApiKey)
                          .toString()
                          .trimmed();
    result.aiBaseUrl = settings
                           .value(
                               QString::fromLatin1(kKeyAiBaseUrl),
                               result.aiBaseUrl)
                           .toString()
                           .trimmed();
    if (result.aiBaseUrl.isEmpty()) {
        result.aiBaseUrl = AppSettings{}.aiBaseUrl;
    }

    result.aiModel = settings
                         .value(
                             QString::fromLatin1(kKeyAiModel),
                             result.aiModel)
                         .toString()
                         .trimmed();
    if (result.aiModel.isEmpty()) {
        result.aiModel = AppSettings{}.aiModel;
    }

    result.aiTimeoutMs = clampAiTimeoutMs(
        settings
            .value(
                QString::fromLatin1(kKeyAiTimeoutMs),
                result.aiTimeoutMs)
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
        QString::fromLatin1(kKeyResultCardDurationMs),
        clampResultCardDurationMs(appSettings.resultCardDurationMs));
    settings.setValue(
        QString::fromLatin1(kKeyResultCardStyle),
        resultCardStyleToString(appSettings.resultCardStyle));
    settings.setValue(
        QString::fromLatin1(kKeyQueryHistoryLimit),
        clampQueryHistoryLimit(appSettings.queryHistoryLimit));
    settings.setValue(
        QString::fromLatin1(kKeyAiAssistEnabled),
        appSettings.aiAssistEnabled);
    settings.setValue(
        QString::fromLatin1(kKeyAiApiKey),
        appSettings.aiApiKey.trimmed());
    settings.setValue(
        QString::fromLatin1(kKeyAiBaseUrl),
        appSettings.aiBaseUrl.trimmed());
    settings.setValue(
        QString::fromLatin1(kKeyAiModel),
        appSettings.aiModel.trimmed());
    settings.setValue(
        QString::fromLatin1(kKeyAiTimeoutMs),
        clampAiTimeoutMs(appSettings.aiTimeoutMs));

    settings.endGroup();
    settings.sync();
}
