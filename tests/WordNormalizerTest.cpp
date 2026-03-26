#include <QtTest/QtTest>

#include "services/WordNormalizer.h"

class WordNormalizerTest : public QObject {
    Q_OBJECT

private slots:
    void normalizeLowercasesAndTrims();
    void normalizeKeepsWordPunctuation();
    void normalizeRejectsNonWordInput();
};

void WordNormalizerTest::normalizeLowercasesAndTrims() {
    const WordNormalizer normalizer;
    QCOMPARE(normalizer.normalizeCandidate(QStringLiteral("   HELLO   ")), QStringLiteral("hello"));
}

void WordNormalizerTest::normalizeKeepsWordPunctuation() {
    const WordNormalizer normalizer;
    QCOMPARE(normalizer.normalizeCandidate(QStringLiteral("*** Co-operate's meaning")),
             QStringLiteral("co-operate's"));
}

void WordNormalizerTest::normalizeRejectsNonWordInput() {
    const WordNormalizer normalizer;
    QVERIFY(normalizer.normalizeCandidate(QStringLiteral("12345 !!!")).isEmpty());
}

QTEST_APPLESS_MAIN(WordNormalizerTest)

#include "WordNormalizerTest.moc"
