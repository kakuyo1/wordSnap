#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

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
    void recognizeAddsTessdataArgumentWhenEngModelExists();
    void recognizeSkipsTessdataArgumentWhenEngModelMissing();
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

void OcrServiceTest::recognizeAddsTessdataArgumentWhenEngModelExists() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString trainedDataPath = QDir(tempDir.path()).filePath(QStringLiteral("eng.traineddata"));
    QFile trainedDataFile(trainedDataPath);
    QVERIFY(trainedDataFile.open(QIODevice::WriteOnly));
    trainedDataFile.write("mock");
    trainedDataFile.close();

    QStringList capturedArguments;
    const OcrService service([&capturedArguments](const QString&, const QStringList& arguments, int, int) {
        capturedArguments = arguments;
        OcrService::ProcessRunResult runResult;
        runResult.started = true;
        runResult.finished = true;
        runResult.normalExit = true;
        runResult.exitCode = 0;
        runResult.standardOutput = QStringLiteral("run");
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), tempDir.path(), &errorMessage);

    QVERIFY(result.success);
    QVERIFY(errorMessage.isEmpty());

    const int tessdataArgIndex = capturedArguments.indexOf(QStringLiteral("--tessdata-dir"));
    QVERIFY(tessdataArgIndex >= 0);
    QCOMPARE(capturedArguments.value(tessdataArgIndex + 1), tempDir.path());
}

void OcrServiceTest::recognizeSkipsTessdataArgumentWhenEngModelMissing() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QStringList capturedArguments;
    const OcrService service([&capturedArguments](const QString&, const QStringList& arguments, int, int) {
        capturedArguments = arguments;
        OcrService::ProcessRunResult runResult;
        runResult.started = true;
        runResult.finished = true;
        runResult.normalExit = true;
        runResult.exitCode = 0;
        runResult.standardOutput = QStringLiteral("run");
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), tempDir.path(), &errorMessage);

    QVERIFY(result.success);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(capturedArguments.indexOf(QStringLiteral("--tessdata-dir")), -1);
}

QTEST_APPLESS_MAIN(OcrServiceTest)

#include "OcrServiceTest.moc"
