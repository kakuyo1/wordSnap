#pragma once

#include <QString>

#include "app/AppTypes.h"
#include "services/StarDictBackend.h"

// Coordinates dictionary backend lifecycle and word lookups.
class DictionaryService final {
public:
    bool initialize(const QString& starDictDirectory, QString* errorMessage = nullptr);

    DictionaryEntry lookup(const QString& normalizedWord) const;

    bool isReady() const;
    QString sourceName() const;

private:
    StarDictBackend backend_;
};
