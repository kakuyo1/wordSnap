#include "services/PhoneticExtractor.h"

#include <QRegularExpression>
#include <QStringList>

namespace {
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

QString joinNonEmptySegments(const QStringList& segments, const int skipIndex) {
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

    auto considerMatch = [&](const QRegularExpression& re, const bool wrapped) {
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
} // namespace

QString PhoneticExtractor::pickPrimaryPhonetic(const DictionaryEntry& entry) {
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

bool PhoneticExtractor::extractInlinePhonetic(const QString& text,
                                              QString* extractedPhonetic,
                                              QString* strippedText) {
    const QString trimmed = text.trimmed();
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
        if (strippedText != nullptr) {
            *strippedText = bracketMatch.captured(2).trimmed();
        }
        return true;
    }

    const QRegularExpressionMatch slashMatch = kLeadingSlash.match(trimmed);
    if (slashMatch.hasMatch()) {
        if (extractedPhonetic != nullptr) {
            *extractedPhonetic = unwrapSinglePhonetic(slashMatch.captured(1));
        }
        if (strippedText != nullptr) {
            *strippedText = slashMatch.captured(2).trimmed();
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
            if (strippedText != nullptr) {
                *strippedText = cleanupPreviewText(joinNonEmptySegments(parts, candidateIndex));
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
            if (strippedText != nullptr) {
                *strippedText = cleanupPreviewText(tokenMatch.captured(2));
            }
            return true;
        }
    }

    return extractPhoneticTokenFromText(trimmed, extractedPhonetic, strippedText);
}
