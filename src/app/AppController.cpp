#include "app/AppController.h"

#include <QCoreApplication>
#include <QCursor>
#include <QDialog>
#include <QImage>
#include <QStringList>
#include <QToolTip>

#include "platform/win/GlobalHotkeyManager.h"
#include "services/DictionaryService.h"
#include "services/ImagePreprocessor.h"
#include "services/OcrService.h"
#include "services/PhoneticExtractor.h"
#include "services/ScreenCaptureService.h"
#include "services/SettingsService.h"
#include "services/WordNormalizer.h"
#include "ui/CaptureOverlay.h"
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

QString firstNonEmpty(const QStringList& lines) {
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            return trimmed;
        }
    }
    return {};
}

QString pickDefinitionPreview(const DictionaryEntry& entry, DisplayMode mode) {
    if (mode == DisplayMode::Zh) {
        QString preview = firstNonEmpty(entry.definitionsZh);
        if (!preview.isEmpty()) {
            return preview;
        }
        preview = firstNonEmpty(entry.definitionsEn);
        if (!preview.isEmpty()) {
            return preview;
        }
        return entry.rawHtml.simplified();
    }

    if (mode == DisplayMode::En) {
        QString preview = firstNonEmpty(entry.definitionsEn);
        if (!preview.isEmpty()) {
            return preview;
        }
        preview = firstNonEmpty(entry.definitionsZh);
        if (!preview.isEmpty()) {
            return preview;
        }
        return entry.rawHtml.simplified();
    }

    const QString zh = firstNonEmpty(entry.definitionsZh);
    const QString en = firstNonEmpty(entry.definitionsEn);
    if (!zh.isEmpty() && !en.isEmpty()) {
        return QStringLiteral("%1 | %2").arg(en, zh);
    }
    if (!en.isEmpty()) {
        return en;
    }
    if (!zh.isEmpty()) {
        return zh;
    }
    return entry.rawHtml.simplified();
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
      trayController_(std::make_unique<TrayController>(this)),
      hotkeyManager_(std::make_unique<GlobalHotkeyManager>(this)),
      captureOverlay_(std::make_unique<CaptureOverlay>()),
      screenCaptureService_(std::make_unique<ScreenCaptureService>()),
      imagePreprocessor_(std::make_unique<ImagePreprocessor>()),
      ocrService_(std::make_unique<OcrService>()),
      wordNormalizer_(std::make_unique<WordNormalizer>()),
      dictionaryService_(std::make_unique<DictionaryService>()),
      resultCardWidget_(std::make_unique<ResultCardWidget>()) {
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
    connect(trayController_.get(), &TrayController::settingsRequested, this, &AppController::onSettingsRequested);
    if (QCoreApplication::instance() != nullptr) {
        connect(trayController_.get(), &TrayController::quitRequested, QCoreApplication::instance(), &QCoreApplication::quit);
    }
    connect(hotkeyManager_.get(), &GlobalHotkeyManager::hotkeyPressed, this, &AppController::onHotkeyPressed);
    connect(captureOverlay_.get(), &CaptureOverlay::regionSelected, this, &AppController::onRegionSelected);
    connect(captureOverlay_.get(), &CaptureOverlay::captureCanceled, this, &AppController::onCaptureCanceled);

    trayController_->initialize(settings_);
    resultCardWidget_->setCardOpacityPercent(settings_.resultCardOpacityPercent);

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
    const QImage captured = screenCaptureService_->capture(globalRect);
    if (captured.isNull()) {
        const QString message = QStringLiteral("Capture failed. Please try again.");
        QToolTip::showText(QCursor::pos(), message);
        resultCardWidget_->showMessage(
            QStringLiteral("wordSnap V1"),
            message,
            QString(),
            QCursor::pos(),
            2200);
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            message,
            1400);
        return;
    }

    const QImage preprocessed = imagePreprocessor_->preprocessForWordRecognition(captured);
    QString ocrError;
    OcrWordResult ocrResult = ocrService_->recognizeSingleWord(preprocessed, settings_.tessdataDir, &ocrError);
    if (!ocrResult.success) {
        const QString message = ocrError.isEmpty()
                                    ? QStringLiteral("OCR failed. Ensure Tesseract is installed.")
                                    : QStringLiteral("OCR failed: %1").arg(ocrError);
        QToolTip::showText(QCursor::pos(), message);
        resultCardWidget_->showMessage(
            QStringLiteral("wordSnap V1"),
            message,
            QString(),
            QCursor::pos(),
            2600);
        trayController_->showInfo(QStringLiteral("wordSnap V1"), message, 2200);
        return;
    }

    ocrResult.normalizedText = wordNormalizer_->normalizeCandidate(ocrResult.rawText);
    if (ocrResult.normalizedText.isEmpty()) {
        const QString message = QStringLiteral("OCR text is not a valid word candidate.");
        QToolTip::showText(QCursor::pos(), message);
        resultCardWidget_->showMessage(
            QStringLiteral("wordSnap V1"),
            message,
            QString(),
            QCursor::pos(),
            2200);
        trayController_->showInfo(
            QStringLiteral("wordSnap V1"),
            QStringLiteral("OCR succeeded, but no valid word token was found."),
            1700);
        return;
    }

    DictionaryEntry entry;
    if (dictionaryService_ != nullptr && dictionaryService_->isReady()) {
        entry = dictionaryService_->lookup(ocrResult.normalizedText);
    }

    if (!entry.found) {
        const QString message = dictionaryService_->isReady()
                                    ? QStringLiteral("OCR: %1 (no dictionary entry)").arg(ocrResult.normalizedText)
                                    : QStringLiteral("OCR: %1 (dictionary unavailable)").arg(ocrResult.normalizedText);
        QToolTip::showText(QCursor::pos(), message);
        resultCardWidget_->showMessage(
            QStringLiteral("OCR"),
            ocrResult.normalizedText,
            QString(),
            QCursor::pos(),
            2800);
        trayController_->showInfo(QStringLiteral("wordSnap V1"), message, 1600);
        return;
    }

    const QString headword = entry.headword.isEmpty() ? ocrResult.normalizedText : entry.headword;
    QString preview = pickDefinitionPreview(entry, settings_.displayMode);
    QString phonetic = PhoneticExtractor::pickPrimaryPhonetic(entry);

    QString extractedPhonetic;
    QString strippedPreview;
    if (PhoneticExtractor::extractInlinePhonetic(preview, &extractedPhonetic, &strippedPreview)) {
        if (phonetic.isEmpty()) {
            phonetic = extractedPhonetic;
        }
        preview = strippedPreview;
    }

    if (phonetic.isEmpty()) {
        QString rawExtractedPhonetic;
        QString ignored;
        if (PhoneticExtractor::extractInlinePhonetic(entry.rawHtml, &rawExtractedPhonetic, &ignored)) {
            phonetic = rawExtractedPhonetic;
        }
    }
    const QString tooltipText = preview.isEmpty()
                                     ? headword
                                     : QStringLiteral("%1\n%2").arg(headword, preview);

    QToolTip::showText(QCursor::pos(), tooltipText);
    resultCardWidget_->showMessage(headword, preview, phonetic, QCursor::pos(), 5000);
    trayController_->showInfo(QStringLiteral("wordSnap V1"), clampTrayMessage(tooltipText), 2500);
}

void AppController::onCaptureCanceled() {
    QToolTip::showText(QCursor::pos(), QStringLiteral("Capture canceled"));
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

    settings_ = updatedSettings;
    resultCardWidget_->setCardOpacityPercent(settings_.resultCardOpacityPercent);
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
