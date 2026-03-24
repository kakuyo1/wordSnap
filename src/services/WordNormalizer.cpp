#include "services/WordNormalizer.h"

#include <QRegularExpression>

QString WordNormalizer::normalizeCandidate(const QString& rawText) const {
    const QString compact = rawText.simplified();
    if (compact.isEmpty()) {
        return {};
    }

    const QRegularExpression tokenPattern(QStringLiteral("[A-Za-z][A-Za-z\\-']*"));
    const QRegularExpressionMatch match = tokenPattern.match(compact);
    if (!match.hasMatch()) {
        return {};
    }

    return match.captured(0).toLower();
}
