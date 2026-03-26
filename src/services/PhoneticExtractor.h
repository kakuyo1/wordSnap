#pragma once

#include <QString>

#include "app/AppTypes.h"

class PhoneticExtractor final {
public:
    static QString pickPrimaryPhonetic(const DictionaryEntry& entry);

    static bool extractInlinePhonetic(const QString& text,
                                      QString* extractedPhonetic,
                                      QString* strippedText);
};
