#include "services/DictionaryService.h"

bool DictionaryService::initialize(const QString& starDictDirectory, QString* errorMessage) {
    return backend_.loadFromDirectory(starDictDirectory, errorMessage);
}

DictionaryEntry DictionaryService::lookup(const QString& normalizedWord) const {
    return backend_.lookup(normalizedWord);
}

bool DictionaryService::isReady() const {
    return backend_.isLoaded();
}

QString DictionaryService::sourceName() const {
    return backend_.bookName();
}
