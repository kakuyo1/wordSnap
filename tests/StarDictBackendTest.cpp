#include <QtTest/QtTest>

#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>

#include "services/StarDictBackend.h"

namespace {
void appendBigEndianUInt32(QByteArray* target, const quint32 value) {
    target->append(static_cast<char>((value >> 24U) & 0xFFU));
    target->append(static_cast<char>((value >> 16U) & 0xFFU));
    target->append(static_cast<char>((value >> 8U) & 0xFFU));
    target->append(static_cast<char>(value & 0xFFU));
}

void appendIdxRecord(QByteArray* idxData,
                     const QByteArray& wordUtf8,
                     const quint32 offset,
                     const quint32 size) {
    idxData->append(wordUtf8);
    idxData->append('\0');
    appendBigEndianUInt32(idxData, offset);
    appendBigEndianUInt32(idxData, size);
}

bool writeFile(const QString& path, const QByteArray& data) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(data) != data.size()) {
        return false;
    }
    return true;
}
} // namespace

class StarDictBackendTest : public QObject {
    Q_OBJECT

private slots:
    void loadFromDirectoryRejectsEmptyPath();
    void loadAndLookupReturnsExpectedEntry();
};

void StarDictBackendTest::loadFromDirectoryRejectsEmptyPath() {
    StarDictBackend backend;
    QString error;

    QVERIFY(!backend.loadFromDirectory(QString(), &error));
    QVERIFY(!error.isEmpty());
}

void StarDictBackendTest::loadAndLookupReturnsExpectedEntry() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString base = tempDir.path() + QStringLiteral("/demo");
    const QString ifoPath = base + QStringLiteral(".ifo");
    const QString idxPath = base + QStringLiteral(".idx");
    const QString dictPath = base + QStringLiteral(".dict");

    const QByteArray definition = QByteArray("to sprint quickly\n")
                                  + QStringLiteral("快速奔跑").toUtf8();

    QByteArray idxData;
    appendIdxRecord(&idxData, QByteArray("run"), 0U, static_cast<quint32>(definition.size()));

    const QByteArray ifoData = QByteArray("StarDict's dict ifo file\n")
                               + QByteArray("version=2.4.2\n")
                               + QByteArray("bookname=demo\n")
                               + QByteArray("wordcount=1\n")
                               + QByteArray("idxfilesize=")
                               + QByteArray::number(idxData.size())
                               + QByteArray("\n")
                               + QByteArray("sametypesequence=m\n");

    QVERIFY(writeFile(ifoPath, ifoData));
    QVERIFY(writeFile(idxPath, idxData));
    QVERIFY(writeFile(dictPath, definition));

    StarDictBackend backend;
    QString error;
    QVERIFY(backend.loadFromDirectory(tempDir.path(), &error));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(backend.isLoaded());
    QCOMPARE(backend.bookName(), QStringLiteral("demo"));

    const DictionaryEntry entry = backend.lookup(QStringLiteral("RUNS"));
    QVERIFY(entry.found);
    QCOMPARE(entry.headword, QStringLiteral("run"));
    QVERIFY(!entry.rawHtml.isEmpty());
    QCOMPARE(entry.definitionsEn.value(0), QStringLiteral("to sprint quickly"));
    QCOMPARE(entry.definitionsZh.value(0), QStringLiteral("快速奔跑"));
}

QTEST_APPLESS_MAIN(StarDictBackendTest)

#include "StarDictBackendTest.moc"
