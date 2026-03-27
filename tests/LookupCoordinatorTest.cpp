#include <QtTest/QtTest>

#include <QImage>

#include "app/LookupCoordinator.h"

class LookupCoordinatorTest : public QObject {
    Q_OBJECT

private slots:
    void runReturnsOcrFailedWhenPipelineDependenciesMissing();
    void runReturnsOcrFailedWhenCaptureIsEmpty();
    void runReturnsOcrFailedWhenRecognizerFails();
    void runReturnsOcrFailedWhenNormalizedCandidateIsEmpty();
    void runReturnsOcrFailedWhenNormalizedCandidateContainsWhitespace();
    void runTrimsNormalizedCandidateBeforeDictionaryLookup();
    void runTrimsDictionaryHeadwordForDisplay();
    void runReturnsDictUnavailableWhenBackendIsNotReady();
    void runReturnsUnknownWhenDictionaryMissesWord();
    void runBuildsFoundResultAndExtractsInlinePhonetic();
};

void LookupCoordinatorTest::runReturnsOcrFailedWhenPipelineDependenciesMissing() {
    LookupCoordinator coordinator(LookupCoordinator::Dependencies{});

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.queryWord, QString());
    QCOMPARE(result.tooltipText,
             QStringLiteral("OCR_FAILED | Internal pipeline is not configured."));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("Internal pipeline is not configured."));
    QCOMPARE(result.trayMessage,
             QStringLiteral("OCR_FAILED | Internal pipeline is not configured."));
    QCOMPARE(result.cardTimeoutMs, 2600);
    QCOMPARE(result.trayTimeoutMs, 2200);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenCaptureIsEmpty() {
    bool preprocessCalled = false;

    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage();
        },
        [&](const QImage&) {
            preprocessCalled = true;
            return QImage(2, 2, QImage::Format_ARGB32);
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("hello");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("hello");
        },
        []() {
            return true;
        },
        [](const QString&) {
            DictionaryEntry entry;
            entry.found = true;
            return entry;
        },
    });

    const LookupCoordinator::Result result =
        coordinator.run(QRect(10, 20, 30, 40), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QVERIFY(result.queryWord.isEmpty());
    QCOMPARE(result.tooltipText, QStringLiteral("OCR_FAILED | Capture failed. Please try again."));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("Capture failed. Please try again."));
    QCOMPARE(result.trayMessage, QStringLiteral("OCR_FAILED | Capture failed. Please try again."));
    QCOMPARE(result.cardTimeoutMs, 2200);
    QCOMPARE(result.trayTimeoutMs, 1400);
    QVERIFY(!preprocessCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenRecognizerFails() {
    bool normalizeCalled = false;

    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString* errorMessage) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("engine unavailable");
            }

            OcrWordResult result;
            result.success = false;
            return result;
        },
        [&](const QString&) {
            normalizeCalled = true;
            return QStringLiteral("hello");
        },
        []() {
            return true;
        },
        [](const QString&) {
            DictionaryEntry entry;
            entry.found = true;
            return entry;
        },
    });

    const LookupCoordinator::Result result =
        coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.queryWord, QString());
    QCOMPARE(result.tooltipText, QStringLiteral("OCR_FAILED | engine unavailable"));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("engine unavailable"));
    QCOMPARE(result.trayMessage, QStringLiteral("OCR_FAILED | engine unavailable"));
    QCOMPARE(result.cardTimeoutMs, 2600);
    QCOMPARE(result.trayTimeoutMs, 2200);
    QVERIFY(!normalizeCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenNormalizedCandidateIsEmpty() {
    bool dictionaryReadyCalled = false;

    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("1234");
            return result;
        },
        [](const QString&) {
            return QString();
        },
        [&]() {
            dictionaryReadyCalled = true;
            return true;
        },
        [](const QString&) {
            DictionaryEntry entry;
            entry.found = true;
            return entry;
        },
    });

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.queryWord, QString());
    QCOMPARE(result.tooltipText,
             QStringLiteral("OCR_FAILED | OCR text is not a valid word candidate."));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("OCR text is not a valid word candidate."));
    QCOMPARE(result.trayMessage,
             QStringLiteral("OCR_FAILED | OCR text is not a valid word candidate."));
    QCOMPARE(result.cardTimeoutMs, 2200);
    QCOMPARE(result.trayTimeoutMs, 1700);
    QVERIFY(!dictionaryReadyCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenNormalizedCandidateContainsWhitespace() {
    bool dictionaryReadyCalled = false;

    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("run fast");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("run fast");
        },
        [&]() {
            dictionaryReadyCalled = true;
            return true;
        },
        [](const QString&) {
            DictionaryEntry entry;
            entry.found = true;
            return entry;
        },
    });

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.queryWord, QString());
    QCOMPARE(result.tooltipText,
             QStringLiteral("OCR_FAILED | OCR text is not a valid word candidate."));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("OCR text is not a valid word candidate."));
    QCOMPARE(result.trayMessage,
             QStringLiteral("OCR_FAILED | OCR text is not a valid word candidate."));
    QCOMPARE(result.cardTimeoutMs, 2200);
    QCOMPARE(result.trayTimeoutMs, 1700);
    QVERIFY(!dictionaryReadyCalled);
}

void LookupCoordinatorTest::runTrimsNormalizedCandidateBeforeDictionaryLookup() {
    QString lookupWord;

    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("RUN");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("  run  ");
        },
        []() {
            return true;
        },
        [&](const QString& word) {
            lookupWord = word;

            DictionaryEntry entry;
            if (word == QStringLiteral("run")) {
                entry.found = true;
                entry.headword = QStringLiteral("run");
                entry.definitionsEn.push_back(QStringLiteral("to move fast"));
            }
            return entry;
        },
    });

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(lookupWord, QStringLiteral("run"));
    QCOMPARE(result.status, LookupCoordinator::Status::Found);
    QCOMPARE(result.queryWord, QStringLiteral("run"));
    QCOMPARE(result.cardTitle, QStringLiteral("run"));
}

void LookupCoordinatorTest::runTrimsDictionaryHeadwordForDisplay() {
    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("run");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("run");
        },
        []() {
            return true;
        },
        [](const QString&) {
            DictionaryEntry entry;
            entry.found = true;
            entry.headword = QStringLiteral("  run  ");
            entry.definitionsEn.push_back(QStringLiteral("to move fast"));
            return entry;
        },
    });

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::Found);
    QCOMPARE(result.cardTitle, QStringLiteral("run"));
    QCOMPARE(result.tooltipText, QStringLiteral("FOUND\nrun\nto move fast"));
}

void LookupCoordinatorTest::runReturnsDictUnavailableWhenBackendIsNotReady() {
    bool lookupCalled = false;

    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("RUN");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("run");
        },
        []() {
            return false;
        },
        [&](const QString&) {
            lookupCalled = true;
            return DictionaryEntry{};
        },
    });

    const LookupCoordinator::Result result =
        coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::DictUnavailable);
    QCOMPARE(result.statusCode, QStringLiteral("DICT_UNAVAILABLE"));
    QCOMPARE(result.queryWord, QStringLiteral("run"));
    QCOMPARE(result.tooltipText, QStringLiteral("DICT_UNAVAILABLE"));
    QCOMPARE(result.cardTitle, QStringLiteral("DICT_UNAVAILABLE"));
    QCOMPARE(result.cardBody, QString());
    QCOMPARE(result.trayMessage, QStringLiteral("DICT_UNAVAILABLE"));
    QCOMPARE(result.cardTimeoutMs, 2600);
    QCOMPARE(result.trayTimeoutMs, 1800);
    QVERIFY(!lookupCalled);
}

void LookupCoordinatorTest::runReturnsUnknownWhenDictionaryMissesWord() {
    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("RUN");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("run");
        },
        []() {
            return true;
        },
        [](const QString&) {
            return DictionaryEntry{};
        },
    });

    const LookupCoordinator::Result result =
        coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::Unknown);
    QCOMPARE(result.statusCode, QStringLiteral("UNKNOWN"));
    QCOMPARE(result.queryWord, QStringLiteral("run"));
    QCOMPARE(result.tooltipText, QStringLiteral("UNKNOWN"));
    QCOMPARE(result.cardTitle, QStringLiteral("UNKNOWN"));
    QCOMPARE(result.cardBody, QString());
    QCOMPARE(result.trayMessage, QStringLiteral("UNKNOWN"));
    QCOMPARE(result.cardTimeoutMs, 2400);
    QCOMPARE(result.trayTimeoutMs, 1600);
}

void LookupCoordinatorTest::runBuildsFoundResultAndExtractsInlinePhonetic() {
    LookupCoordinator coordinator(LookupCoordinator::Dependencies{
        [](const QRect&) {
            return QImage(4, 4, QImage::Format_ARGB32);
        },
        [](const QImage& image) {
            return image;
        },
        [](const QImage&, const QString&, QString*) {
            OcrWordResult result;
            result.success = true;
            result.rawText = QStringLiteral("hello");
            return result;
        },
        [](const QString&) {
            return QStringLiteral("hello");
        },
        []() {
            return true;
        },
        [](const QString&) {
            DictionaryEntry entry;
            entry.found = true;
            entry.headword = QStringLiteral("hello");
            entry.definitionsEn.push_back(QStringLiteral("[həˈləʊ] | interj. used as greeting"));
            return entry;
        },
    });

    const LookupCoordinator::Result result =
        coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::Found);
    QCOMPARE(result.statusCode, QStringLiteral("FOUND"));
    QCOMPARE(result.queryWord, QStringLiteral("hello"));
    QCOMPARE(result.cardTitle, QStringLiteral("hello"));
    QCOMPARE(result.cardBody, QStringLiteral("interj. used as greeting"));
    QCOMPARE(result.cardPhonetic, QStringLiteral("həˈləʊ"));
    QCOMPARE(result.tooltipText, QStringLiteral("FOUND\nhello\ninterj. used as greeting"));
    QCOMPARE(result.trayMessage, QStringLiteral("FOUND | hello | interj. used as greeting"));
    QCOMPARE(result.cardTimeoutMs, 5000);
    QCOMPARE(result.trayTimeoutMs, 2500);
}

QTEST_APPLESS_MAIN(LookupCoordinatorTest)

#include "LookupCoordinatorTest.moc"
