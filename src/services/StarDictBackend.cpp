#include "services/StarDictBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

namespace {
bool containsCjk(const QString& text) {
    static const QRegularExpression kCjkRegex(QStringLiteral("[\\x{4E00}-\\x{9FFF}\\x{3400}-\\x{4DBF}]"));
    return kCjkRegex.match(text).hasMatch();
}

QString firstMeaningfulLine(const QString& definition) {
    const QStringList lines = definition.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            return trimmed;
        }
    }
    return definition.trimmed();
}
} // namespace

bool StarDictBackend::loadFromDirectory(const QString& directoryPath, QString* errorMessage) {
    directoryPath_.clear();
    bookName_.clear();
    sameTypeSequence_.clear();
    indexByLowerWord_.clear();
    dictData_.clear();

    const QString trimmedDirectory = directoryPath.trimmed();
    if (trimmedDirectory.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("StarDict directory is empty.");
        }
        return false;
    }

    QDir dir(trimmedDirectory);
    if (!dir.exists()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("StarDict directory does not exist: %1").arg(trimmedDirectory);
        }
        return false;
    }

    const QStringList ifoFiles = dir.entryList(QStringList() << QStringLiteral("*.ifo"), QDir::Files | QDir::Readable, QDir::Name);
    if (ifoFiles.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No .ifo file found in: %1").arg(trimmedDirectory);
        }
        return false;
    }

    const QString ifoPath = dir.filePath(ifoFiles.first());
    const QFileInfo ifoInfo(ifoPath);
    const QString basePath = ifoInfo.absolutePath() + QLatin1Char('/') + ifoInfo.completeBaseName();
    const QString idxPath = basePath + QStringLiteral(".idx");
    const QString dictPath = basePath + QStringLiteral(".dict");

    QFile ifoFile(ifoPath);
    if (!ifoFile.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open .ifo file: %1").arg(ifoPath);
        }
        return false;
    }

    const QStringList ifoLines = QString::fromUtf8(ifoFile.readAll()).split('\n');
    for (const QString& rawLine : ifoLines) {
        const QString line = rawLine.trimmed();
        const int separator = line.indexOf('=');
        if (separator <= 0) {
            continue;
        }

        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();
        if (key == QStringLiteral("bookname")) {
            bookName_ = value;
        } else if (key == QStringLiteral("sametypesequence")) {
            sameTypeSequence_ = value;
        }
    }

    QFile idxFile(idxPath);
    if (!idxFile.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open .idx file: %1").arg(idxPath);
        }
        return false;
    }
    const QByteArray idxData = idxFile.readAll();
    if (idxData.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Index file is empty: %1").arg(idxPath);
        }
        return false;
    }

    int cursor = 0;
    while (cursor < idxData.size()) {
        const int zeroPos = idxData.indexOf('\0', cursor);
        if (zeroPos < 0) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Malformed .idx record (missing separator).");
            }
            indexByLowerWord_.clear();
            return false;
        }

        const QByteArray wordBytes = idxData.mid(cursor, zeroPos - cursor);
        cursor = zeroPos + 1;

        if (cursor + 8 > idxData.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Malformed .idx record (missing offset/size).");
            }
            indexByLowerWord_.clear();
            return false;
        }

        const uchar* meta = reinterpret_cast<const uchar*>(idxData.constData() + cursor);
        const quint32 offset = readBigEndianUInt32(meta);
        const quint32 size = readBigEndianUInt32(meta + 4);
        cursor += 8;

        const QString headword = QString::fromUtf8(wordBytes).trimmed();
        if (headword.isEmpty()) {
            continue;
        }

        const QString key = headword.toLower();
        if (!indexByLowerWord_.contains(key)) {
            EntryRecord record;
            record.offset = offset;
            record.size = size;
            record.headword = headword;
            indexByLowerWord_.insert(key, record);
        }
    }

    if (indexByLowerWord_.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No index entries loaded from: %1").arg(idxPath);
        }
        return false;
    }

    QFile dictFile(dictPath);
    if (!dictFile.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open .dict file: %1").arg(dictPath);
        }
        indexByLowerWord_.clear();
        return false;
    }
    dictData_ = dictFile.readAll();
    if (dictData_.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Dictionary data file is empty: %1").arg(dictPath);
        }
        indexByLowerWord_.clear();
        return false;
    }

    directoryPath_ = trimmedDirectory;
    if (bookName_.isEmpty()) {
        bookName_ = ifoInfo.completeBaseName();
    }
    return true;
}

DictionaryEntry StarDictBackend::lookup(const QString& normalizedWord) const {
    DictionaryEntry entry;

    const QString key = normalizedWord.trimmed().toLower();
    if (key.isEmpty() || !isLoaded()) {
        return entry;
    }

    auto it = indexByLowerWord_.constFind(key);
    if (it == indexByLowerWord_.constEnd() && key.endsWith(QStringLiteral("'s"))) {
        it = indexByLowerWord_.constFind(key.left(key.size() - 2));
    }
    if (it == indexByLowerWord_.constEnd() && key.endsWith(QLatin1Char('s'))) {
        it = indexByLowerWord_.constFind(key.left(key.size() - 1));
    }
    if (it == indexByLowerWord_.constEnd()) {
        return entry;
    }

    const EntryRecord& record = it.value();
    const quint64 endOffset = static_cast<quint64>(record.offset) + static_cast<quint64>(record.size);
    if (record.size == 0 || endOffset > static_cast<quint64>(dictData_.size())) {
        return entry;
    }

    const QByteArray payload = dictData_.mid(static_cast<int>(record.offset), static_cast<int>(record.size));
    const QString definition = extractDefinition(payload);
    if (definition.isEmpty()) {
        return entry;
    }

    entry.headword = record.headword;
    entry.rawHtml = definition;
    entry.found = true;

    const QStringList lines = definition.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        if (containsCjk(trimmed)) {
            entry.definitionsZh.push_back(trimmed);
        } else {
            entry.definitionsEn.push_back(trimmed);
        }
    }

    if (entry.definitionsZh.isEmpty() && entry.definitionsEn.isEmpty()) {
        entry.definitionsEn.push_back(firstMeaningfulLine(definition));
    }

    return entry;
}

bool StarDictBackend::isLoaded() const {
    return !indexByLowerWord_.isEmpty() && !dictData_.isEmpty();
}

QString StarDictBackend::bookName() const {
    return bookName_;
}

quint32 StarDictBackend::readBigEndianUInt32(const uchar* data) {
    const quint32 b0 = static_cast<quint32>(data[0]) << 24U;
    const quint32 b1 = static_cast<quint32>(data[1]) << 16U;
    const quint32 b2 = static_cast<quint32>(data[2]) << 8U;
    const quint32 b3 = static_cast<quint32>(data[3]);
    return b0 | b1 | b2 | b3;
}

QString StarDictBackend::decodeText(const QByteArray& payload) {
    QByteArray normalized = payload;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');
    return QString::fromUtf8(normalized).trimmed();
}

QString StarDictBackend::extractDefinition(const QByteArray& payload) const {
    if (payload.isEmpty()) {
        return {};
    }

    if (!sameTypeSequence_.isEmpty()) {
        if (sameTypeSequence_.size() == 1) {
            return decodeText(payload);
        }

        QByteArray flattened = payload;
        flattened.replace('\0', '\n');
        return decodeText(flattened);
    }

    if (payload.size() == 1) {
        return {};
    }

    const QByteArray body = payload.mid(1);
    const int firstNull = body.indexOf('\0');
    if (firstNull >= 0) {
        return decodeText(body.left(firstNull));
    }

    return decodeText(body);
}
