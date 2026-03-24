#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>

#include "app/AppTypes.h"

// Minimal StarDict backend for .ifo + .idx + .dict lookup.
class StarDictBackend final {
public:
    bool loadFromDirectory(const QString& directoryPath, QString* errorMessage = nullptr);

    DictionaryEntry lookup(const QString& normalizedWord) const;

    bool isLoaded() const;
    QString bookName() const;

private:
    struct EntryRecord {
        quint32 offset{0};
        quint32 size{0};
        QString headword;
    };

    static quint32 readBigEndianUInt32(const uchar* data);
    static QString decodeText(const QByteArray& payload);

    QString extractDefinition(const QByteArray& payload) const;

    QString directoryPath_;
    QString bookName_;
    QString sameTypeSequence_;
    QHash<QString, EntryRecord> indexByLowerWord_;
    QByteArray dictData_;
};
