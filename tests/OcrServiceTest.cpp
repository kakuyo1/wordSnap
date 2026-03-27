#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>

#include "services/OcrService.h"
#include "services/TesseractExecutableResolver.h"

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
    void recognizeFailsWhenProcessExitCodeIsNonZero();
    void recognizeFailsWhenOcrReturnsNoText();
    void recognizeSucceedsWhenOcrReturnsText();
    void recognizeAddsTessdataArgumentWhenEngModelExists();
    void recognizeSkipsTessdataArgumentWhenEngModelMissing();
    void resolvePrefersAppLocalAndEnvVariables();
    void resolveFallsBackToPathEntries();
    void resolveUsesDiscoveredExecutableBeforeCommonLocations();
    void resolveUsesCommonInstallLocationWhenDiscoveredMissing();
    void resolveAcceptsExecutablePathInTesseractPathEnv();
    void resolveFallsBackToCommandWhenNoCandidates();
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

void OcrServiceTest::recognizeFailsWhenProcessExitCodeIsNonZero() {
    const OcrService service([](const QString&, const QStringList&, int, int) {
        OcrService::ProcessRunResult runResult;
        runResult.started = true;
        runResult.finished = true;
        runResult.normalExit = true;
        runResult.exitCode = 1;
        runResult.standardError.clear();
        return runResult;
    });

    QString errorMessage;
    const OcrWordResult result = service.recognizeSingleWord(makeTinyImage(), QString(), &errorMessage);

    QVERIFY(!result.success);
    QCOMPARE(errorMessage, QStringLiteral("Tesseract process failed."));
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

void OcrServiceTest::resolvePrefersAppLocalAndEnvVariables() {
    const QSet<QString> existingPaths{
        QStringLiteral("C:/app/tesseract.exe"),
        QStringLiteral("C:/env/tesseract.exe"),
        QStringLiteral("C:/env-path/tesseract.exe")
    };
    const TesseractExecutableResolver resolver([&existingPaths](const QString& path) {
        return existingPaths.contains(path);
    });

    TesseractExecutableResolver::RuntimeSnapshot snapshot;
    snapshot.appDirPath = QStringLiteral("C:/app");
    snapshot.tesseractExeEnv = QStringLiteral("C:/env/tesseract.exe");
    snapshot.tesseractPathEnv = QStringLiteral("C:/env-path");
    snapshot.systemPath = QStringLiteral("C:/path-a;C:/path-b");

    QCOMPARE(resolver.resolve(snapshot), QStringLiteral("C:/app/tesseract.exe"));
}

void OcrServiceTest::resolveFallsBackToPathEntries() {
    const QSet<QString> existingPaths{
        QStringLiteral("C:/path-b/tesseract.exe")
    };
    const TesseractExecutableResolver resolver([&existingPaths](const QString& path) {
        return existingPaths.contains(path);
    });

    TesseractExecutableResolver::RuntimeSnapshot snapshot;
    snapshot.appDirPath = QStringLiteral("C:/app");
    snapshot.systemPath = QStringLiteral(" ; C:/path-a ; C:/path-b ; ");

    QCOMPARE(resolver.resolve(snapshot), QStringLiteral("C:/path-b/tesseract.exe"));
}

void OcrServiceTest::resolveUsesDiscoveredExecutableBeforeCommonLocations() {
    const QSet<QString> existingPaths{
        QStringLiteral("C:/Program Files/Tesseract-OCR/tesseract.exe")
    };
    const TesseractExecutableResolver resolver([&existingPaths](const QString& path) {
        return existingPaths.contains(path);
    });

    TesseractExecutableResolver::RuntimeSnapshot snapshot;
    snapshot.appDirPath = QStringLiteral("C:/app");
    snapshot.discoveredTesseract = QStringLiteral("C:/portable/tesseract.exe");

    QCOMPARE(resolver.resolve(snapshot), QStringLiteral("C:/portable/tesseract.exe"));
}

void OcrServiceTest::resolveUsesCommonInstallLocationWhenDiscoveredMissing() {
    const QSet<QString> existingPaths{
        QStringLiteral("C:/Program Files/Tesseract-OCR/tesseract.exe")
    };
    const TesseractExecutableResolver resolver([&existingPaths](const QString& path) {
        return existingPaths.contains(path);
    });

    TesseractExecutableResolver::RuntimeSnapshot snapshot;
    snapshot.appDirPath = QStringLiteral("C:/app");

    QCOMPARE(resolver.resolve(snapshot), QStringLiteral("C:/Program Files/Tesseract-OCR/tesseract.exe"));
}

void OcrServiceTest::resolveAcceptsExecutablePathInTesseractPathEnv() {
    const QSet<QString> existingPaths{
        QStringLiteral("C:/custom/bin/tesseract.exe")
    };
    const TesseractExecutableResolver resolver([&existingPaths](const QString& path) {
        return existingPaths.contains(path);
    });

    TesseractExecutableResolver::RuntimeSnapshot snapshot;
    snapshot.appDirPath = QStringLiteral("C:/app");
    snapshot.tesseractPathEnv = QStringLiteral("C:/custom/bin/tesseract.exe");

    QCOMPARE(resolver.resolve(snapshot), QStringLiteral("C:/custom/bin/tesseract.exe"));
}

void OcrServiceTest::resolveFallsBackToCommandWhenNoCandidates() {
    const TesseractExecutableResolver resolver([](const QString&) {
        return false;
    });

    TesseractExecutableResolver::RuntimeSnapshot snapshot;
    snapshot.appDirPath = QStringLiteral("C:/app");
    snapshot.systemPath = QStringLiteral("C:/path-a;C:/path-b");

    QCOMPARE(resolver.resolve(snapshot), QStringLiteral("tesseract"));
}

QTEST_APPLESS_MAIN(OcrServiceTest)

#include "OcrServiceTest.moc"
