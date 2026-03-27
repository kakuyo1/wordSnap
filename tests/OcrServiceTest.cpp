#include <QtTest/QtTest>

#include "services/OcrService.h"

namespace {
QImage makeTinyImage() {
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::white);
    return image;
}
} // namespace

class OcrServiceTest : public QObject {
    Q_OBJECT

private slots:
    void recognizeFailsWhenProcessDoesNotStart();
    void recognizeFailsWhenProcessTimesOut();
    void recognizeFailsWhenOcrReturnsNoText();
    void recognizeSucceedsWhenOcrReturnsText();
};

void OcrServiceTest::recognizeFailsWhenProcessDoesNotStart() {
    const OcrService service([](const QString&, const QStringList&, int, int) {
        OcrService::ProcessRunResult runResult;
        runResult.started = false;
        runResult.startError = QStringLiteral("mock-start-error");
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), QString(), &errorMessage);

    QVERIFY(!result.success);
    QVERIFY(errorMessage.contains(QStringLiteral("Tesseract did not start")));
    QVERIFY(errorMessage.contains(QStringLiteral("mock-start-error")));
}

void OcrServiceTest::recognizeFailsWhenProcessTimesOut() {
    const OcrService service([](const QString&, const QStringList&, int, int) {
        OcrService::ProcessRunResult runResult;
        runResult.started = true;
        runResult.finished = false;
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), QString(), &errorMessage);

    QVERIFY(!result.success);
    QCOMPARE(errorMessage, QStringLiteral("Tesseract timed out."));
}

void OcrServiceTest::recognizeFailsWhenOcrReturnsNoText() {
    const OcrService service([](const QString&, const QStringList&, int, int) {
        OcrService::ProcessRunResult runResult;
        runResult.started = true;
        runResult.finished = true;
        runResult.normalExit = true;
        runResult.exitCode = 0;
        runResult.standardError = QStringLiteral("ocr-empty");
        runResult.standardOutput = QString();
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), QString(), &errorMessage);

    QVERIFY(!result.success);
    QCOMPARE(errorMessage, QStringLiteral("ocr-empty"));
}

void OcrServiceTest::recognizeSucceedsWhenOcrReturnsText() {
    const OcrService service([](const QString&, const QStringList&, int, int) {
        OcrService::ProcessRunResult runResult;
        runResult.started = true;
        runResult.finished = true;
        runResult.normalExit = true;
        runResult.exitCode = 0;
        runResult.standardOutput = QStringLiteral("  Hello  ");
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), QString(), &errorMessage);

    QVERIFY(result.success);
    QCOMPARE(result.rawText, QStringLiteral("Hello"));
    QVERIFY(errorMessage.isEmpty());
}

QTEST_APPLESS_MAIN(OcrServiceTest)

#include "OcrServiceTest.moc"
