#include <QtTest/QtTest>

#include <QDateTime>
#include <QFileInfo>
#include <QTemporaryDir>

#include "services/QueryHistoryService.h"

namespace {
QueryHistoryRecord makeRecord(const QString& statusCode,
                              const QString& queryWord,
                              const QString& headword,
                              const QString& preview,
                              const QString& phonetic,
                              const QDateTime& timestampUtc) {
    QueryHistoryRecord record;
    record.timestampUtc = timestampUtc;
    record.statusCode = statusCode;
    record.queryWord = queryWord;
    record.headword = headword;
    record.preview = preview;
    record.phonetic = phonetic;
    return record;
}
} // namespace

class QueryHistoryServiceTest : public QObject {
    Q_OBJECT

private slots:
    void appendAndLoadRecentKeepsNewestFirst();
    void appendTrimsToConfiguredMaximum();
    void clearRemovesHistoryFile();
    void appendRejectsEmptyStatusCode();
};

void QueryHistoryServiceTest::appendAndLoadRecentKeepsNewestFirst() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.path() + QStringLiteral("/history.jsonl");
    QueryHistoryService service(filePath, kMinQueryHistoryLimit);

    QVERIFY(service.append(makeRecord(
        QStringLiteral("FOUND"),
        QStringLiteral("hello"),
        QStringLiteral("hello"),
        QStringLiteral("interj. greeting"),
        QStringLiteral("həˈləʊ"),
        QDateTime::fromString(QStringLiteral("2026-03-26T08:00:00.000Z"), Qt::ISODateWithMs))));
    QVERIFY(service.append(makeRecord(
        QStringLiteral("UNKNOWN"),
        QStringLiteral("xylophobize"),
        QString(),
        QString(),
        QString(),
        QDateTime::fromString(QStringLiteral("2026-03-26T08:10:00.000Z"), Qt::ISODateWithMs))));

    const QVector<QueryHistoryRecord> records = service.loadRecent();
    QCOMPARE(records.size(), 2);

    QCOMPARE(records.at(0).statusCode, QStringLiteral("UNKNOWN"));
    QCOMPARE(records.at(0).queryWord, QStringLiteral("xylophobize"));
    QCOMPARE(records.at(1).statusCode, QStringLiteral("FOUND"));
    QCOMPARE(records.at(1).headword, QStringLiteral("hello"));
}

void QueryHistoryServiceTest::appendTrimsToConfiguredMaximum() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.path() + QStringLiteral("/history.jsonl");
    QueryHistoryService service(filePath, kMinQueryHistoryLimit);

    for (int index = 0; index < kMinQueryHistoryLimit + 5; ++index) {
        const QString word = QStringLiteral("word%1").arg(index);
        const QDateTime timestamp =
            QDateTime::fromString(QStringLiteral("2026-03-26T08:00:00.000Z"), Qt::ISODateWithMs).addSecs(index);
        QVERIFY(service.append(makeRecord(
            QStringLiteral("FOUND"),
            word,
            word,
            QStringLiteral("preview%1").arg(index),
            QString(),
            timestamp)));
    }

    const QVector<QueryHistoryRecord> records = service.loadRecent();
    QCOMPARE(records.size(), kMinQueryHistoryLimit);
    QCOMPARE(records.front().queryWord, QStringLiteral("word%1").arg(kMinQueryHistoryLimit + 4));
    QCOMPARE(records.back().queryWord, QStringLiteral("word5"));
}

void QueryHistoryServiceTest::clearRemovesHistoryFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.path() + QStringLiteral("/history.jsonl");
    QueryHistoryService service(filePath, kMinQueryHistoryLimit);

    QVERIFY(service.append(makeRecord(
        QStringLiteral("FOUND"),
        QStringLiteral("run"),
        QStringLiteral("run"),
        QStringLiteral("v. move quickly"),
        QString(),
        QDateTime::currentDateTimeUtc())));

    QVERIFY(QFileInfo::exists(filePath));

    QString errorMessage;
    QVERIFY(service.clear(&errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QVERIFY(!QFileInfo::exists(filePath));

    const QVector<QueryHistoryRecord> records = service.loadRecent();
    QVERIFY(records.isEmpty());
}

void QueryHistoryServiceTest::appendRejectsEmptyStatusCode() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.path() + QStringLiteral("/history.jsonl");
    QueryHistoryService service(filePath, kMinQueryHistoryLimit);

    QueryHistoryRecord invalidRecord;
    invalidRecord.timestampUtc = QDateTime::currentDateTimeUtc();
    invalidRecord.queryWord = QStringLiteral("hello");

    QString errorMessage;
    QVERIFY(!service.append(invalidRecord, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(service.loadRecent().isEmpty());
}

QTEST_APPLESS_MAIN(QueryHistoryServiceTest)

#include "QueryHistoryServiceTest.moc"
