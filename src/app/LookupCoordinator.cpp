#include "app/LookupCoordinator.h"

#include <QStringList>

#include <utility>

#include "services/PhoneticExtractor.h"

namespace {
constexpr auto kStatusFound = "FOUND";
constexpr auto kStatusOcrFailed = "OCR_FAILED";
constexpr auto kStatusUnknown = "UNKNOWN";
constexpr auto kStatusDictUnavailable = "DICT_UNAVAILABLE";

QString statusCodeFor(const LookupCoordinator::Status status) {
    switch (status) {
    case LookupCoordinator::Status::Found:
        return QString::fromLatin1(kStatusFound);
    case LookupCoordinator::Status::OcrFailed:
        return QString::fromLatin1(kStatusOcrFailed);
    case LookupCoordinator::Status::Unknown:
        return QString::fromLatin1(kStatusUnknown);
    case LookupCoordinator::Status::DictUnavailable:
        return QString::fromLatin1(kStatusDictUnavailable);
    }

    return QString::fromLatin1(kStatusOcrFailed);
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

QString pickDefinitionPreview(const DictionaryEntry& entry, const DisplayMode mode) {
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

LookupCoordinator::Result makeStatusResult(const LookupCoordinator::Status status,
                                           QString queryWord,
                                           QString detail,
                                           const int cardTimeoutMs,
                                           const int trayTimeoutMs) {
    LookupCoordinator::Result result;
    result.status = status;
    result.statusCode = statusCodeFor(status);
    result.queryWord = queryWord.trimmed();

    detail = detail.trimmed();
    const QString message = detail.isEmpty()
                                ? result.statusCode
                                : QStringLiteral("%1 | %2").arg(result.statusCode, detail);

    result.tooltipText = message;
    result.cardTitle = result.statusCode;
    result.cardBody = detail;
    result.cardPhonetic.clear();
    result.trayMessage = message;
    result.cardTimeoutMs = cardTimeoutMs;
    result.trayTimeoutMs = trayTimeoutMs;
    return result;
}
} // namespace

LookupCoordinator::LookupCoordinator(Dependencies dependencies)
    : dependencies_(std::move(dependencies)) {
}

LookupCoordinator::Result LookupCoordinator::run(const QRect& globalRect,
                                                 const QString& tessdataDir,
                                                 const DisplayMode displayMode) const {
    if (!dependencies_.capture || !dependencies_.preprocess || !dependencies_.recognizeSingleWord
        || !dependencies_.normalizeCandidate || !dependencies_.isDictionaryReady || !dependencies_.lookup) {
        return makeStatusResult(
            Status::OcrFailed,
            QString(),
            QStringLiteral("Internal pipeline is not configured."),
            2600,
            2200);
    }

    const QImage captured = dependencies_.capture(globalRect);
    if (captured.isNull()) {
        return makeStatusResult(
            Status::OcrFailed,
            QString(),
            QStringLiteral("Capture failed. Please try again."),
            2200,
            1400);
    }

    const QImage preprocessed = dependencies_.preprocess(captured);
    QString ocrError;
    OcrWordResult ocrResult = dependencies_.recognizeSingleWord(preprocessed, tessdataDir, &ocrError);
    if (!ocrResult.success) {
        const QString detail = ocrError.isEmpty()
                                   ? QStringLiteral("Ensure Tesseract is installed.")
                                   : ocrError;
        return makeStatusResult(
            Status::OcrFailed,
            QString(),
            detail,
            2600,
            2200);
    }

    ocrResult.normalizedText = dependencies_.normalizeCandidate(ocrResult.rawText);
    if (ocrResult.normalizedText.isEmpty()) {
        return makeStatusResult(
            Status::OcrFailed,
            QString(),
            QStringLiteral("OCR text is not a valid word candidate."),
            2200,
            1700);
    }

    if (!dependencies_.isDictionaryReady()) {
        return makeStatusResult(
            Status::DictUnavailable,
            ocrResult.normalizedText,
            QString(),
            2600,
            1800);
    }

    const DictionaryEntry entry = dependencies_.lookup(ocrResult.normalizedText);
    if (!entry.found) {
        return makeStatusResult(
            Status::Unknown,
            ocrResult.normalizedText,
            QString(),
            2400,
            1600);
    }

    const QString headword = entry.headword.isEmpty() ? ocrResult.normalizedText : entry.headword;
    QString preview = pickDefinitionPreview(entry, displayMode);
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

    QString tooltipText = QStringLiteral("%1\n%2")
                              .arg(QString::fromLatin1(kStatusFound), headword);
    if (!preview.isEmpty()) {
        tooltipText += QStringLiteral("\n") + preview;
    }

    QString trayMessage = QStringLiteral("%1 | %2")
                              .arg(QString::fromLatin1(kStatusFound), headword);
    if (!preview.isEmpty()) {
        trayMessage += QStringLiteral(" | ") + preview;
    }

    LookupCoordinator::Result result;
    result.status = Status::Found;
    result.statusCode = QString::fromLatin1(kStatusFound);
    result.queryWord = ocrResult.normalizedText;
    result.tooltipText = tooltipText;
    result.cardTitle = headword;
    result.cardBody = preview;
    result.cardPhonetic = phonetic;
    result.trayMessage = clampTrayMessage(trayMessage);
    result.cardTimeoutMs = 5000;
    result.trayTimeoutMs = 2500;
    return result;
}
