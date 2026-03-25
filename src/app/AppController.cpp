#include "app/AppController.h"

#include <QCoreApplication>
#include <QCursor>
#include <QDialog>
#include <QImage>
#include <QRegularExpression>
#include <QStringList>
#include <QToolTip>

#include "platform/win/GlobalHotkeyManager.h"
#include "services/DictionaryService.h"
#include "services/ImagePreprocessor.h"
#include "services/OcrService.h"
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

QString pickPhonetic(const DictionaryEntry& entry) {
    const QString merged = entry.phonetic.trimmed();
    if (!merged.isEmpty()) {
        return merged;
    }

    const QString uk = entry.phoneticUk.trimmed();
    const QString us = entry.phoneticUs.trimmed();
    if (!uk.isEmpty() && !us.isEmpty()) {
        if (uk.compare(us, Qt::CaseInsensitive) == 0) {
            return uk;
        }
        return QStringLiteral("UK %1 | US %2").arg(uk, us);
    }
    if (!uk.isEmpty()) {
        return uk;
    }
    return us;
}

QString unwrapSinglePhonetic(QString text) {
    text = text.trimmed();
    if (text.size() >= 2) {
        if (text.startsWith('[') && text.endsWith(']')) {
            text = text.mid(1, text.size() - 2).trimmed();
        } else if (text.startsWith('/') && text.endsWith('/')) {
            text = text.mid(1, text.size() - 2).trimmed();
        }
    }
    return text;
}

bool looksLikePhoneticSegment(const QString& segment) {
    const QString trimmed = segment.trimmed();
    if (trimmed.isEmpty() || trimmed.size() > 96) {
        return false;
    }

    static const QRegularExpression kCjkChars(QStringLiteral("[\\x{4E00}-\\x{9FFF}\\x{3400}-\\x{4DBF}]"));
    if (kCjkChars.match(trimmed).hasMatch()) {
        return false;
    }

    if ((trimmed.startsWith('[') && trimmed.endsWith(']'))
        || (trimmed.startsWith('/') && trimmed.endsWith('/'))) {
        return true;
    }

    static const QRegularExpression kIpaLikeChars(
        QStringLiteral("[ˈˌˊˋːəәæɑɒɔɜɪʊʌθðŋʃʒɚɝ]"));
    return kIpaLikeChars.match(trimmed).hasMatch();
}

QString joinNonEmptySegments(const QStringList& segments, int skipIndex) {
    QStringList normalized;
    normalized.reserve(segments.size());
    for (int i = 0; i < segments.size(); ++i) {
        if (i == skipIndex) {
            continue;
        }

        const QString part = segments.at(i).trimmed();
        if (!part.isEmpty()) {
            normalized.push_back(part);
        }
    }
    return normalized.join(QStringLiteral(" | "));
}

QString cleanupPreviewText(QString text) {
    text = text.trimmed();

    static const QRegularExpression kDuplicatedPipe(QStringLiteral("\\s*\\|\\s*\\|\\s*"));
    text.replace(kDuplicatedPipe, QStringLiteral(" | "));

    while (text.startsWith('|') || text.startsWith(';')) {
        text.remove(0, 1);
        text = text.trimmed();
    }
    while (text.endsWith('|') || text.endsWith(';')) {
        text.chop(1);
        text = text.trimmed();
    }
    return text;
}

bool extractPhoneticTokenFromText(const QString& text,
                                  QString* extractedPhonetic,
                                  QString* strippedText) {
    static const QRegularExpression kBracketToken(QStringLiteral("\\[([^\\]\\n]{1,64})\\]"));
    static const QRegularExpression kSlashToken(QStringLiteral("/([^/\\n]{1,64})/"));
    static const QRegularExpression kIpaToken(
        QStringLiteral("([A-Za-z,.']{0,12}[ˈˌˊˋːəәæɑɒɔɜɪʊʌθðŋʃʒɚɝ][A-Za-zˈˌˊˋːəәæɑɒɔɜɪʊʌθðŋʃʒɚɝ,.']{0,20})"));

    QRegularExpressionMatch bestMatch;
    int bestStart = -1;
    int bestLength = 0;
    QString bestPhonetic;

    auto considerMatch = [&](const QRegularExpression& re, bool wrapped) {
        const QRegularExpressionMatch match = re.match(text);
        if (!match.hasMatch()) {
            return;
        }

        const int start = match.capturedStart(0);
        const int length = match.capturedLength(0);
        if (start < 0 || length <= 0) {
            return;
        }

        if (bestStart >= 0 && start >= bestStart) {
            return;
        }

        bestMatch = match;
        bestStart = start;
        bestLength = length;
        bestPhonetic = wrapped ? match.captured(1).trimmed() : match.captured(0).trimmed();
    };

    considerMatch(kBracketToken, true);
    considerMatch(kSlashToken, true);
    considerMatch(kIpaToken, false);

    if (bestStart < 0 || bestPhonetic.isEmpty()) {
        return false;
    }

    if (extractedPhonetic != nullptr) {
        *extractedPhonetic = unwrapSinglePhonetic(bestPhonetic);
    }

    if (strippedText != nullptr) {
        QString withoutToken = text;
        withoutToken.remove(bestStart, bestLength);
        *strippedText = cleanupPreviewText(withoutToken);
    }
    return true;
}

bool tryExtractInlinePhonetic(const QString& preview,
                              QString* extractedPhonetic,
                              QString* strippedPreview) {
    const QString trimmed = preview.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    static const QRegularExpression kLeadingBracket(
        QStringLiteral("^\\s*\\[([^\\]\\n]{1,96})\\]\\s*(?:\\|\\s*)?(.*)$"));
    static const QRegularExpression kLeadingSlash(
        QStringLiteral("^\\s*/([^/\\n]{1,96})/\\s*(?:\\|\\s*)?(.*)$"));

    const QRegularExpressionMatch bracketMatch = kLeadingBracket.match(trimmed);
    if (bracketMatch.hasMatch()) {
        if (extractedPhonetic != nullptr) {
            *extractedPhonetic = unwrapSinglePhonetic(bracketMatch.captured(1));
        }
        if (strippedPreview != nullptr) {
            *strippedPreview = bracketMatch.captured(2).trimmed();
        }
        return true;
    }

    const QRegularExpressionMatch slashMatch = kLeadingSlash.match(trimmed);
    if (slashMatch.hasMatch()) {
        if (extractedPhonetic != nullptr) {
            *extractedPhonetic = unwrapSinglePhonetic(slashMatch.captured(1));
        }
        if (strippedPreview != nullptr) {
            *strippedPreview = slashMatch.captured(2).trimmed();
        }
        return true;
    }

    const QStringList parts = trimmed.split('|');
    if (parts.size() >= 2) {
        int candidateIndex = -1;
        QString candidateText;

        for (int i = 0; i < parts.size(); ++i) {
            const QString segment = parts.at(i).trimmed();
            if (!looksLikePhoneticSegment(segment)) {
                continue;
            }

            if (candidateIndex < 0 || segment.size() < candidateText.size()) {
                candidateIndex = i;
                candidateText = segment;
            }
        }

        if (candidateIndex >= 0) {
            if (extractedPhonetic != nullptr) {
                *extractedPhonetic = unwrapSinglePhonetic(candidateText);
            }
            if (strippedPreview != nullptr) {
                *strippedPreview = cleanupPreviewText(joinNonEmptySegments(parts, candidateIndex));
            }
            return true;
        }
    }

    static const QRegularExpression kLeadingToken(
        QStringLiteral("^\\s*([^\\s|]{2,80})\\s+(.+)$"));
    const QRegularExpressionMatch tokenMatch = kLeadingToken.match(trimmed);
    if (tokenMatch.hasMatch()) {
        const QString candidate = tokenMatch.captured(1).trimmed();
        if (looksLikePhoneticSegment(candidate)) {
            if (extractedPhonetic != nullptr) {
                *extractedPhonetic = unwrapSinglePhonetic(candidate);
            }
            if (strippedPreview != nullptr) {
                *strippedPreview = cleanupPreviewText(tokenMatch.captured(2));
            }
            return true;
        }
    }

    return extractPhoneticTokenFromText(trimmed, extractedPhonetic, strippedPreview);
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
    QString phonetic = pickPhonetic(entry);

    QString extractedPhonetic;
    QString strippedPreview;
    if (tryExtractInlinePhonetic(preview, &extractedPhonetic, &strippedPreview)) {
        if (phonetic.isEmpty()) {
            phonetic = extractedPhonetic;
        }
        preview = strippedPreview;
    }

    if (phonetic.isEmpty()) {
        QString rawExtractedPhonetic;
        QString ignored;
        if (tryExtractInlinePhonetic(entry.rawHtml, &rawExtractedPhonetic, &ignored)) {
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
