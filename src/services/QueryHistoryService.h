#pragma once

#include <QString>
#include <QVector>

#include "app/AppTypes.h"

// Persists query history to local JSONL for replay/filtering.
class QueryHistoryService final {
public:
    explicit QueryHistoryService(QString filePath = QString(),
                                 int maxEntries = kDefaultQueryHistoryLimit);

    QVector<QueryHistoryRecord> loadRecent(int limit = -1, QString* errorMessage = nullptr) const;
    bool append(const QueryHistoryRecord& record, QString* errorMessage = nullptr);
    bool clear(QString* errorMessage = nullptr) const;

    void setMaxEntries(int maxEntries);
    int maxEntries() const;

private:
    QString filePath_;
    int maxEntries_{kDefaultQueryHistoryLimit};

    QVector<QueryHistoryRecord> readAll(QString* errorMessage) const;
    bool writeAll(const QVector<QueryHistoryRecord>& records, QString* errorMessage) const;
};
