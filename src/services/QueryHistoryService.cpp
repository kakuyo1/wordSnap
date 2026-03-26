#include "services/QueryHistoryService.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextStream>

#include <utility>

namespace {
QString resolveDefaultHistoryFilePath() {
    QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataRoot.trimmed().isEmpty()) {
        dataRoot = QDir::homePath() + QStringLiteral("/.wordSnapV1");
    }

    return QDir(dataRoot).filePath(QStringLiteral("query_history.jsonl"));
}

QJsonObject toJsonObject(const QueryHistoryRecord& record) {
    QJsonObject json;
    json.insert(QStringLiteral("timestamp_utc"), record.timestampUtc.toString(Qt::ISODateWithMs));
    json.insert(QStringLiteral("status"), record.statusCode);
    json.insert(QStringLiteral("query_word"), record.queryWord);
    json.insert(QStringLiteral("headword"), record.headword);
    json.insert(QStringLiteral("preview"), record.preview);
    json.insert(QStringLiteral("phonetic"), record.phonetic);
    return json;
}

bool fromJsonObject(const QJsonObject& json, QueryHistoryRecord* record) {
    if (record == nullptr) {
        return false;
    }

    const QString statusCode = json.value(QStringLiteral("status")).toString().trimmed().toUpper();
    if (statusCode.isEmpty()) {
        return false;
    }

    QueryHistoryRecord parsed;
    parsed.timestampUtc = QDateTime::fromString(
        json.value(QStringLiteral("timestamp_utc")).toString(),
        Qt::ISODateWithMs);
    if (parsed.timestampUtc.isValid()) {
        parsed.timestampUtc = parsed.timestampUtc.toUTC();
    } else {
        parsed.timestampUtc = QDateTime::currentDateTimeUtc();
    }

    parsed.statusCode = statusCode;
    parsed.queryWord = json.value(QStringLiteral("query_word")).toString();
    parsed.headword = json.value(QStringLiteral("headword")).toString();
    parsed.preview = json.value(QStringLiteral("preview")).toString();
    parsed.phonetic = json.value(QStringLiteral("phonetic")).toString();

    *record = parsed;
    return true;
}
} // namespace

QueryHistoryService::QueryHistoryService(QString filePath, const int maxEntries)
    : filePath_(std::move(filePath)) {
    if (filePath_.trimmed().isEmpty()) {
        filePath_ = resolveDefaultHistoryFilePath();
    }
    setMaxEntries(maxEntries);
}

QVector<QueryHistoryRecord> QueryHistoryService::loadRecent(const int limit, QString* errorMessage) const {
    QVector<QueryHistoryRecord> records = readAll(errorMessage);
    const int effectiveLimit = limit > 0 ? clampQueryHistoryLimit(limit) : maxEntries_;
    if (records.size() > effectiveLimit) {
        records.resize(effectiveLimit);
    }
    return records;
}

bool QueryHistoryService::append(const QueryHistoryRecord& record, QString* errorMessage) {
    QVector<QueryHistoryRecord> records = readAll(errorMessage);

    QueryHistoryRecord normalized = record;
    normalized.statusCode = normalized.statusCode.trimmed().toUpper();
    if (normalized.statusCode.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("History record status cannot be empty.");
        }
        return false;
    }

    if (!normalized.timestampUtc.isValid()) {
        normalized.timestampUtc = QDateTime::currentDateTimeUtc();
    } else {
        normalized.timestampUtc = normalized.timestampUtc.toUTC();
    }

    records.prepend(normalized);
    if (records.size() > maxEntries_) {
        records.resize(maxEntries_);
    }

    return writeAll(records, errorMessage);
}

bool QueryHistoryService::clear(QString* errorMessage) const {
    const QFileInfo info(filePath_);
    if (!info.exists()) {
        return true;
    }

    QFile file(filePath_);
    if (file.remove()) {
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Failed to clear history file: %1").arg(file.errorString());
    }
    return false;
}

void QueryHistoryService::setMaxEntries(const int maxEntries) {
    maxEntries_ = clampQueryHistoryLimit(maxEntries);
}

int QueryHistoryService::maxEntries() const {
    return maxEntries_;
}

QVector<QueryHistoryRecord> QueryHistoryService::readAll(QString* errorMessage) const {
    QVector<QueryHistoryRecord> records;

    QFile file(filePath_);
    if (!file.exists()) {
        return records;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to read history: %1").arg(file.errorString());
        }
        return records;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            continue;
        }

        QueryHistoryRecord record;
        if (fromJsonObject(document.object(), &record)) {
            records.push_back(record);
        }
    }

    return records;
}

bool QueryHistoryService::writeAll(const QVector<QueryHistoryRecord>& records, QString* errorMessage) const {
    const QFileInfo info(filePath_);
    QDir directory = info.dir();
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to create history directory: %1").arg(directory.path());
        }
        return false;
    }

    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to write history: %1").arg(file.errorString());
        }
        return false;
    }

    QTextStream stream(&file);
    for (const QueryHistoryRecord& record : records) {
        const QJsonDocument doc(toJsonObject(record));
        stream << doc.toJson(QJsonDocument::Compact) << '\n';
    }

    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to flush history file.");
        }
        return false;
    }

    return true;
}
