#pragma once

#include <QString>

// Normalizes OCR text into a lookup-friendly word token.
class WordNormalizer final {
public:
    QString normalizeCandidate(const QString& rawText) const;
};
