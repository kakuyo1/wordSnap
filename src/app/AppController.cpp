#include "app/AppController.h"

#include <QCoreApplication>
#include <QCursor>
#include <QDateTime>
#include <QDialog>
#include <QImage>
#include <QToolTip>

#include "app/LookupCoordinator.h"
#include "services/AiAssistService.h"
#include "platform/win/GlobalHotkeyManager.h"
#include "services/DictionaryService.h"
#include "services/ImagePreprocessor.h"
#include "services/OcrService.h"
#include "services/QueryHistoryService.h"
#include "services/ScreenCaptureService.h"
#include "services/SettingsService.h"
#include "services/WordNormalizer.h"
#include "ui/CaptureOverlay.h"
#include "ui/HistoryDialog.h"
#include "ui/ResultCardWidget.h"
#include "ui/SettingsDialog.h"
#include "ui/TrayController.h"

namespace {
void appendWarningMessage(QString* warningMessage, const QString& warning) {
    if (warningMessage == nullptr || warning.isEmpty()) {
        return;
    }

    if (warningMessage->isEmpty()) {
        *warningMessage = warning;
    } else {
        *warningMessage += QStringLiteral("\n") + warning;
    }
}

QString clampTrayMessage(QString message, int maxChars = 180) {
    message = message.simplified();
    if (message.size() <= maxChars) {
        return message;
    }
    return message.left(maxChars - 3) + QStringLiteral("...");
}
} // namespace

AppController::AppController(QObject* parent)
    : QObject(parent),
      settingsService_(std::make_unique<SettingsService>()),
      queryHistoryService_(std::make_unique<QueryHistoryService>()),
      trayController_(std::make_unique<TrayController>(this)),
      hotkeyManager_(std::make_unique<GlobalHotkeyManager>(this)),
      captureOverlay_(std::make_unique<CaptureOverlay>()),
      screenCaptureService_(std::make_unique<ScreenCaptureService>()),
      imagePreprocessor_(std::make_unique<ImagePreprocessor>()),
      ocrService_(std::make_unique<OcrService>()),
      wordNormalizer_(std::make_unique<WordNormalizer>()),
      dictionaryService_(std::make_unique<DictionaryService>()),
      resultCardWidget_(std::make_unique<ResultCardWidget>()),
      aiAssistService_(std::make_unique<AiAssistService>()),
      lookupCoordinator_(std::make_unique<LookupCoordinator>(LookupCoordinator::Dependencies{
          [this](const QRect& rect) {
              return screenCaptureService_->capture(rect);
          },
          [this](const QImage& image) {
              return imagePreprocessor_->preprocessForWordRecognition(image);
          },
          [this](const QImage& image, const QString& tessdataDir, QString* errorMessage) {
              return ocrService_->recognizeSingleWord(image, tessdataDir, errorMessage);
          },
          [this](const QString& rawText) {
              return wordNormalizer_->normalizeCandidate(rawText);
          },
          [this]() {
              return dictionaryService_ != nullptr && dictionaryService_->isReady();
          },
          [this](const QString& normalizedWord) {
              if (dictionaryService_ == nullptr) {
                  return DictionaryEntry{};
              }
              return dictionaryService_->lookup(normalizedWord);
          },
      })) {
}

AppController::~AppController() = default;

bool AppController::initialize(QString* warningMessage) {
    if (!TrayController::isSystemTraySupported()) {
        if (warningMessage != nullptr) {
            *warningMessage = QStringLiteral("System tray is not available.");
        }
        return false;
    }

    settings_ = settingsService_->load();

    connect(trayController_.get(), &TrayController::captureRequested, this, &AppController::onHotkeyPressed);
    connect(trayController_.get(), &TrayController::historyRequested, this, &AppController::onHistoryRequested);
    connect(trayController_.get(), &TrayController::settingsRequested, this, &AppController::onSettingsRequested);
    if (QCoreApplication::instance() != nullptr) {
        connect(trayController_.get(), &TrayController::quitRequested, QCoreApplication::instance(), &QCoreApplication::quit);
    }
    connect(hotkeyManager_.get(), &GlobalHotkeyManager::hotkeyPressed, this, &AppController::onHotkeyPressed);
    connect(captureOverlay_.get(), &CaptureOverlay::regionSelected, this, &AppController::onRegionSelected);
    connect(captureOverlay_.get(), &CaptureOverlay::captureCanceled, this, &AppController::onCaptureCanceled);

    trayController_->initialize(settings_);
    resultCardWidget_->setCardOpacityPercent(settings_.resultCardOpacityPercent);
    resultCardWidget_->setCardStyle(settings_.resultCardStyle);
    if (queryHistoryService_ != nullptr) {
        queryHistoryService_->setMaxEntries(settings_.queryHistoryLimit);
    }

    if (aiAssistService_ != nullptr) {
        const QString aiWarning = aiAssistService_->applySettings(settings_);
        if (!aiWarning.isEmpty() && settings_.aiAssistEnabled) {
            const QString warning = QStringLiteral("AI assist is unavailable: %1").arg(aiWarning);
            appendWarningMessage(warningMessage, warning);
            trayController_->showInfo(
                QStringLiteral("wordSnap V1"),
                clampTrayMessage(warning),
                2200);
        }
    }

    QString dictionaryError;
    if (!dictionaryService_->initialize(settings_.starDictDir, &dictionaryError)) {
        const QString warning = QStringLiteral("Dictionary unavailable: %1").arg(dictionaryError);
        appendWarningMessage(warningMessage, warning);
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            QStringLiteral("Dictionary not loaded. OCR-only mode."),
            1700);
    }

    QString hotkeyError;
    if (!hotkeyManager_->registerHotkey(settings_.hotkey, &hotkeyError)) {
        const QString fallbackHotkey = QStringLiteral("Ctrl+Alt+S");
        QString fallbackError;
        if (settings_.hotkey.compare(fallbackHotkey, Qt::CaseInsensitive) != 0
            && hotkeyManager_->registerHotkey(fallbackHotkey, &fallbackError)) {
            settings_.hotkey = fallbackHotkey;
            settingsService_->save(settings_);
            trayController_->setHotkeyDisplay(settings_.hotkey);

            const QString warning = QStringLiteral("Configured hotkey unavailable. Switched to %1.")
                                        .arg(settings_.hotkey);
            QToolTip::showText(QCursor::pos(), warning);
            trayController_->showInfo(QStringLiteral("wordSnap V1"), warning, 2600);
            appendWarningMessage(warningMessage, warning);
        } else {
            const QString warning = QStringLiteral("Global hotkey registration failed: %1")
                                        .arg(hotkeyError);
            QToolTip::showText(QCursor::pos(), warning);
            trayController_->showInfo(QStringLiteral("wordSnap V1"), warning, 2400);
            appendWarningMessage(warningMessage, warning);
        }
    } else {
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            QStringLiteral("Global hotkey ready: %1").arg(settings_.hotkey),
            1500);
    }

    return true;
}

void AppController::onHotkeyPressed() {
    if (captureOverlay_ != nullptr && !captureOverlay_->isCapturing()) {
        captureOverlay_->beginCapture();
    }
}

void AppController::onRegionSelected(const QRect& globalRect) {
    if (lookupCoordinator_ == nullptr) {
        return;
    }

    ++activeLookupToken_;
    const int lookupToken = activeLookupToken_;

    const LookupCoordinator::Result result =
        lookupCoordinator_->run(globalRect, settings_.tessdataDir, settings_.displayMode);

    const QPoint anchorPos = QCursor::pos();
    QToolTip::showText(anchorPos, result.tooltipText);
    resultCardWidget_->showMessage(
        result.statusCode,
        result.cardTitle,
        result.cardBody,
        result.cardPhonetic,
        anchorPos,
        settings_.resultCardDurationMs);
    trayController_->showInfo(QStringLiteral("wordSnap V1"), result.trayMessage, result.trayTimeoutMs);

    appendHistoryRecord(
        result.statusCode,
        result.queryWord,
        result.cardTitle,
        result.cardBody,
        result.cardPhonetic);

    if (result.status != LookupCoordinator::Status::Found || aiAssistService_ == nullptr || resultCardWidget_ == nullptr) {
        return;
    }

    if (!settings_.aiAssistEnabled) {
        return;
    }

    if (!aiAssistService_->isAvailable()) {
        resultCardWidget_->showAiError(QStringLiteral("AI 获取失败，不影响基础查词。"));
        return;
    }

    QString aiWord = result.cardTitle.trimmed();
    if (aiWord.isEmpty()) {
        aiWord = result.queryWord.trimmed();
    }
    if (aiWord.isEmpty()) {
        resultCardWidget_->showAiError(QStringLiteral("AI 获取失败，不影响基础查词。"));
        return;
    }

    resultCardWidget_->showAiLoading();
    aiAssistService_->requestWordAssist(aiWord, [this, lookupToken](const AiAssistService::Result& aiResult) {
        if (lookupToken != activeLookupToken_ || resultCardWidget_ == nullptr) {
            return;
        }

        if (aiResult.status == AiAssistService::Result::Status::Success) {
            resultCardWidget_->showAiContent(aiResult.content);
            return;
        }

        resultCardWidget_->showAiError(QStringLiteral("AI 获取失败，不影响基础查词。"));
    });
}

void AppController::onCaptureCanceled() {
    QToolTip::showText(QCursor::pos(), QStringLiteral("Capture canceled"));
}

void AppController::onHistoryRequested() {
    if (queryHistoryService_ == nullptr) {
        return;
    }

    QString loadError;
    const QVector<QueryHistoryRecord> records = queryHistoryService_->loadRecent(settings_.queryHistoryLimit, &loadError);
    if (!loadError.isEmpty()) {
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            clampTrayMessage(QStringLiteral("History load warning: %1").arg(loadError)),
            2600);
    }

    HistoryDialog dialog(records);
    const int dialogResult = dialog.exec();

    if (dialog.clearRequested()) {
        QString clearError;
        if (queryHistoryService_->clear(&clearError)) {
            trayController_->showInfo(
                QStringLiteral("wordSnap V1"),
                QStringLiteral("History cleared."),
                1600);
        } else {
            const QString warning = QStringLiteral("History clear failed: %1").arg(clearError);
            QToolTip::showText(QCursor::pos(), warning);
            trayController_->showInfo(QStringLiteral("wordSnap V1"), clampTrayMessage(warning), 2600);
        }
    }

    if (dialogResult == QDialog::Accepted && dialog.hasReviewSelection()) {
        showHistoryRecord(dialog.reviewSelection());
    }
}

void AppController::appendHistoryRecord(const QString& statusCode,
                                        const QString& queryWord,
                                        const QString& headword,
                                        const QString& preview,
                                        const QString& phonetic) {
    if (queryHistoryService_ == nullptr) {
        return;
    }

    QueryHistoryRecord record;
    record.timestampUtc = QDateTime::currentDateTimeUtc();
    record.statusCode = statusCode;
    record.queryWord = queryWord;
    record.headword = headword;
    record.preview = preview;
    record.phonetic = phonetic;

    QString historyError;
    if (!queryHistoryService_->append(record, &historyError) && !historyError.isEmpty()) {
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            clampTrayMessage(QStringLiteral("History save warning: %1").arg(historyError)),
            2200);
    }
}

void AppController::showHistoryRecord(const QueryHistoryRecord& record) {
    const QString statusCode = record.statusCode.trimmed().toUpper();
    const QString headword = record.headword.trimmed();
    const QString queryWord = record.queryWord.trimmed();
    const QString displayWord = headword.isEmpty() ? queryWord : headword;
    const QString preview = record.preview.trimmed();

    QString tooltip = statusCode;
    if (!displayWord.isEmpty()) {
        tooltip += QStringLiteral("\n") + displayWord;
    }
    if (!preview.isEmpty()) {
        tooltip += QStringLiteral("\n") + preview;
    }

    const QPoint anchorPos = QCursor::pos();
    QToolTip::showText(anchorPos, tooltip);
    resultCardWidget_->showMessage(statusCode, displayWord, preview, record.phonetic, anchorPos, settings_.resultCardDurationMs);

    QString trayMessage = statusCode;
    if (!displayWord.isEmpty()) {
        trayMessage += QStringLiteral(" | ") + displayWord;
    }
    if (!preview.isEmpty()) {
        trayMessage += QStringLiteral(" | ") + preview;
    }
    trayController_->showInfo(QStringLiteral("wordSnap V1"), clampTrayMessage(trayMessage), 2200);
}

void AppController::onSettingsRequested() {
    SettingsDialog dialog(settings_);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const AppSettings requestedSettings = dialog.editedSettings();
    AppSettings updatedSettings = settings_;
    QString warningMessage;

    const QString requestedHotkey = requestedSettings.hotkey.trimmed();
    if (requestedHotkey.compare(settings_.hotkey, Qt::CaseInsensitive) != 0) {
        QString hotkeyError;
        if (hotkeyManager_->registerHotkey(requestedHotkey, &hotkeyError)) {
            updatedSettings.hotkey = requestedHotkey;
            trayController_->setHotkeyDisplay(updatedSettings.hotkey);
        } else {
            QString rollbackError;
            hotkeyManager_->registerHotkey(settings_.hotkey, &rollbackError);
            appendWarningMessage(
                &warningMessage,
                QStringLiteral("Hotkey was not updated: %1").arg(hotkeyError));
        }
    }

    const QString defaultStarDictDir = QCoreApplication::applicationDirPath() + QStringLiteral("/dict");
    QString requestedStarDictDir = requestedSettings.starDictDir.trimmed();
    if (requestedStarDictDir.isEmpty()) {
        requestedStarDictDir = defaultStarDictDir;
    }
    if (requestedStarDictDir != settings_.starDictDir) {
        QString dictionaryError;
        if (dictionaryService_->initialize(requestedStarDictDir, &dictionaryError)) {
            updatedSettings.starDictDir = requestedStarDictDir;
        } else {
            QString rollbackError;
            if (!settings_.starDictDir.isEmpty()) {
                dictionaryService_->initialize(settings_.starDictDir, &rollbackError);
            }
            appendWarningMessage(
                &warningMessage,
                QStringLiteral("Dictionary path was not updated: %1").arg(dictionaryError));
        }
    }

    updatedSettings.displayMode = requestedSettings.displayMode;
    updatedSettings.tessdataDir = requestedSettings.tessdataDir.trimmed();
    updatedSettings.resultCardOpacityPercent = requestedSettings.resultCardOpacityPercent;
    updatedSettings.resultCardDurationMs = requestedSettings.resultCardDurationMs;
    updatedSettings.resultCardStyle = requestedSettings.resultCardStyle;
    updatedSettings.queryHistoryLimit = requestedSettings.queryHistoryLimit;
    updatedSettings.aiAssistEnabled = requestedSettings.aiAssistEnabled;
    updatedSettings.aiApiKey = requestedSettings.aiApiKey;
    updatedSettings.aiBaseUrl = requestedSettings.aiBaseUrl;
    updatedSettings.aiModel = requestedSettings.aiModel;
    updatedSettings.aiTimeoutMs = requestedSettings.aiTimeoutMs;

    settings_ = updatedSettings;
    resultCardWidget_->setCardOpacityPercent(settings_.resultCardOpacityPercent);
    resultCardWidget_->setCardStyle(settings_.resultCardStyle);
    if (queryHistoryService_ != nullptr) {
        queryHistoryService_->setMaxEntries(settings_.queryHistoryLimit);
    }
    if (aiAssistService_ != nullptr) {
        const QString aiWarning = aiAssistService_->applySettings(settings_);
        if (!aiWarning.isEmpty() && settings_.aiAssistEnabled) {
            appendWarningMessage(
                &warningMessage,
                QStringLiteral("AI assist is unavailable: %1").arg(aiWarning));
        }
    }
    ++activeLookupToken_;
    settingsService_->save(settings_);

    if (warningMessage.isEmpty()) {
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            QStringLiteral("Settings saved."),
            1500);
        return;
    }

    QToolTip::showText(QCursor::pos(), warningMessage);
    trayController_->showInfo(
        QStringLiteral("wordSnap V1"),
        clampTrayMessage(warningMessage),
        3200);
}
