#include <QtTest/QtTest>

#include "app/AppTypes.h"
#include "services/PhoneticExtractor.h"

class PhoneticExtractorTest : public QObject {
    Q_OBJECT

private slots:
    void pickPrimaryPhoneticPrefersMergedField();
    void pickPrimaryPhoneticCombinesUkAndUs();
    void extractInlinePhoneticFromLeadingBracket();
    void extractInlinePhoneticFromPipeSeparatedSegments();
};

void PhoneticExtractorTest::pickPrimaryPhoneticPrefersMergedField() {
    DictionaryEntry entry;
    entry.phonetic = QStringLiteral("/həˈləʊ/");
    entry.phoneticUk = QStringLiteral("/dummy-uk/");
    entry.phoneticUs = QStringLiteral("/dummy-us/");

    QCOMPARE(PhoneticExtractor::pickPrimaryPhonetic(entry), QStringLiteral("/həˈləʊ/"));
}

void PhoneticExtractorTest::pickPrimaryPhoneticCombinesUkAndUs() {
    DictionaryEntry entry;
    entry.phoneticUk = QStringLiteral("/təˈmɑːtəʊ/");
    entry.phoneticUs = QStringLiteral("/təˈmeɪtoʊ/");

    QCOMPARE(PhoneticExtractor::pickPrimaryPhonetic(entry),
             QStringLiteral("UK /təˈmɑːtəʊ/ | US /təˈmeɪtoʊ/"));
}

void PhoneticExtractorTest::extractInlinePhoneticFromLeadingBracket() {
    QString extracted;
    QString stripped;

    const bool ok = PhoneticExtractor::extractInlinePhonetic(
        QStringLiteral("[həˈləʊ] | interj. used as greeting"),
        &extracted,
        &stripped);

    QVERIFY(ok);
    QCOMPARE(extracted, QStringLiteral("həˈləʊ"));
    QCOMPARE(stripped, QStringLiteral("interj. used as greeting"));
}

void PhoneticExtractorTest::extractInlinePhoneticFromPipeSeparatedSegments() {
    QString extracted;
    QString stripped;

    const bool ok = PhoneticExtractor::extractInlinePhonetic(
        QStringLiteral("v. move quickly | /rʌn/ | 跑"),
        &extracted,
        &stripped);

    QVERIFY(ok);
    QCOMPARE(extracted, QStringLiteral("rʌn"));
    QCOMPARE(stripped, QStringLiteral("v. move quickly | 跑"));
}

QTEST_APPLESS_MAIN(PhoneticExtractorTest)

#include "PhoneticExtractorTest.moc"
