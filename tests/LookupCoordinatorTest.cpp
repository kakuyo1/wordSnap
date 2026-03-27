#include <QtTest/QtTest>

#include <QImage>

#include "app/LookupCoordinator.h"
#include "support/LookupCoordinatorFixture.h"

class LookupCoordinatorTest : public QObject {
    Q_OBJECT

private slots:
    void runReturnsOcrFailedWhenPipelineDependenciesMissing();
    void runReturnsOcrFailedWhenCaptureIsEmpty();
    void runReturnsOcrFailedWhenPreprocessImageIsEmpty();
    void runReturnsOcrFailedWhenRecognizerFails();
    void runReturnsOcrFailedWithDefaultHintWhenRecognizerErrorMissing();
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
    LookupCoordinatorFixture fixture;
    fixture.capturedImage = QImage();
    LookupCoordinator coordinator = fixture.createCoordinator();

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
    QVERIFY(!fixture.preprocessCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenPreprocessImageIsEmpty() {
    LookupCoordinatorFixture fixture;
    fixture.preprocessedImage = QImage();
    LookupCoordinator coordinator = fixture.createCoordinator();

    const LookupCoordinator::Result result =
        coordinator.run(QRect(10, 20, 30, 40), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QVERIFY(result.queryWord.isEmpty());
    QCOMPARE(result.tooltipText, QStringLiteral("OCR_FAILED | Preprocess failed. Please try again."));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("Preprocess failed. Please try again."));
    QCOMPARE(result.trayMessage, QStringLiteral("OCR_FAILED | Preprocess failed. Please try again."));
    QCOMPARE(result.cardTimeoutMs, 2200);
    QCOMPARE(result.trayTimeoutMs, 1400);
    QVERIFY(fixture.preprocessCalled);
    QVERIFY(!fixture.recognizeCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenRecognizerFails() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.success = false;
    fixture.ocrError = QStringLiteral("engine unavailable");
    LookupCoordinator coordinator = fixture.createCoordinator();

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
    QVERIFY(!fixture.normalizeCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWithDefaultHintWhenRecognizerErrorMissing() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.success = false;
    fixture.ocrError.clear();
    LookupCoordinator coordinator = fixture.createCoordinator();

    const LookupCoordinator::Result result =
        coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::OcrFailed);
    QCOMPARE(result.statusCode, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.queryWord, QString());
    QCOMPARE(result.tooltipText, QStringLiteral("OCR_FAILED | Ensure Tesseract is installed."));
    QCOMPARE(result.cardTitle, QStringLiteral("OCR_FAILED"));
    QCOMPARE(result.cardBody, QStringLiteral("Ensure Tesseract is installed."));
    QCOMPARE(result.trayMessage, QStringLiteral("OCR_FAILED | Ensure Tesseract is installed."));
    QCOMPARE(result.cardTimeoutMs, 2600);
    QCOMPARE(result.trayTimeoutMs, 2200);
    QVERIFY(fixture.preprocessCalled);
    QVERIFY(fixture.recognizeCalled);
    QVERIFY(!fixture.normalizeCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenNormalizedCandidateIsEmpty() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.rawText = QStringLiteral("1234");
    fixture.normalizedCandidate.clear();
    LookupCoordinator coordinator = fixture.createCoordinator();

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
    QVERIFY(!fixture.dictionaryReadyCalled);
}

void LookupCoordinatorTest::runReturnsOcrFailedWhenNormalizedCandidateContainsWhitespace() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.rawText = QStringLiteral("run fast");
    fixture.normalizedCandidate = QStringLiteral("run fast");
    LookupCoordinator coordinator = fixture.createCoordinator();

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
    QVERIFY(!fixture.dictionaryReadyCalled);
}

void LookupCoordinatorTest::runTrimsNormalizedCandidateBeforeDictionaryLookup() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.rawText = QStringLiteral("RUN");
    fixture.normalizedCandidate = QStringLiteral("  run  ");
    fixture.dictionaryEntry = LookupCoordinatorFixture::makeFoundEntry(
        QStringLiteral("run"),
        QStringLiteral("to move fast"));
    LookupCoordinator coordinator = fixture.createCoordinator();

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(fixture.lookupWord, QStringLiteral("run"));
    QCOMPARE(result.status, LookupCoordinator::Status::Found);
    QCOMPARE(result.queryWord, QStringLiteral("run"));
    QCOMPARE(result.cardTitle, QStringLiteral("run"));
}

void LookupCoordinatorTest::runTrimsDictionaryHeadwordForDisplay() {
    LookupCoordinatorFixture fixture;
    fixture.dictionaryEntry = LookupCoordinatorFixture::makeFoundEntry(
        QStringLiteral("  run  "),
        QStringLiteral("to move fast"));
    LookupCoordinator coordinator = fixture.createCoordinator();

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());

    QCOMPARE(result.status, LookupCoordinator::Status::Found);
    QCOMPARE(result.cardTitle, QStringLiteral("run"));
    QCOMPARE(result.tooltipText, QStringLiteral("FOUND\nrun\nto move fast"));
}

void LookupCoordinatorTest::runReturnsDictUnavailableWhenBackendIsNotReady() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.rawText = QStringLiteral("RUN");
    fixture.dictionaryReady = false;
    LookupCoordinator coordinator = fixture.createCoordinator();

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
    QVERIFY(!fixture.lookupCalled);
}

void LookupCoordinatorTest::runReturnsUnknownWhenDictionaryMissesWord() {
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.rawText = QStringLiteral("RUN");
    fixture.dictionaryEntry = DictionaryEntry{};
    LookupCoordinator coordinator = fixture.createCoordinator();

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
    LookupCoordinatorFixture fixture;
    fixture.ocrResult.rawText = QStringLiteral("hello");
    fixture.normalizedCandidate = QStringLiteral("hello");
    fixture.dictionaryEntry = LookupCoordinatorFixture::makeFoundEntry(
        QStringLiteral("hello"),
        QStringLiteral("[həˈləʊ] | interj. used as greeting"));
    LookupCoordinator coordinator = fixture.createCoordinator();

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
