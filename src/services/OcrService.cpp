#include "services/OcrService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <utility>

namespace {
OcrService::ProcessRunResult runWithQProcess(const QString& executable,
                                             const QStringList& arguments,
                                             const int startTimeoutMs,
                                             const int finishTimeoutMs) {
    OcrService::ProcessRunResult runResult;

    QProcess process;
    process.start(executable, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted(startTimeoutMs)) {
        runResult.startError = process.errorString();
        return runResult;
    }

    runResult.started = true;
    if (!process.waitForFinished(finishTimeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        runResult.finished = false;
        return runResult;
    }

    runResult.finished = true;
    runResult.normalExit = process.exitStatus() == QProcess::NormalExit;
    runResult.exitCode = process.exitCode();
    runResult.standardError = QString::fromUtf8(process.readAllStandardError()).trimmed();
    runResult.standardOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return runResult;
}

QString executableFromPathEntry(const QString& rawEntry) {
    const QString entry = rawEntry.trimmed();
    if (entry.isEmpty()) {
        return {};
    }

    QFileInfo info(entry);
    if (info.exists() && info.isFile() && info.fileName().compare(QStringLiteral("tesseract.exe"), Qt::CaseInsensitive) == 0) {
        return info.absoluteFilePath();
    }

    if (info.exists() && info.isDir()) {
        const QString candidate = QDir(info.absoluteFilePath()).filePath(QStringLiteral("tesseract.exe"));
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

QString resolveTesseractExecutable() {
    const QString appLocal = QCoreApplication::applicationDirPath() + QStringLiteral("/tesseract.exe");
    if (QFileInfo::exists(appLocal)) {
        return appLocal;
    }

    const QString envExe = executableFromPathEntry(qEnvironmentVariable("TESSERACT_EXE"));
    if (!envExe.isEmpty()) {
        return envExe;
    }

    const QString envPath = executableFromPathEntry(qEnvironmentVariable("TESSERACT_PATH"));
    if (!envPath.isEmpty()) {
        return envPath;
    }

    const QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    if (processEnvironment.contains(QStringLiteral("PATH"))) {
        const QStringList pathEntries = processEnvironment.value(QStringLiteral("PATH")).split(';', Qt::SkipEmptyParts);
        for (const QString& pathEntry : pathEntries) {
            const QString candidate = executableFromPathEntry(pathEntry);
            if (!candidate.isEmpty()) {
                return candidate;
            }
        }
    }

    const QString discoveredExe = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
    if (!discoveredExe.isEmpty()) {
        return discoveredExe;
    }

    const QString discoveredExeWithSuffix = QStandardPaths::findExecutable(QStringLiteral("tesseract.exe"));
    if (!discoveredExeWithSuffix.isEmpty()) {
        return discoveredExeWithSuffix;
    }

    const QStringList commonInstallLocations{
        QStringLiteral("C:/Program Files/Tesseract-OCR/tesseract.exe"),
        QStringLiteral("C:/Program Files (x86)/Tesseract-OCR/tesseract.exe")
    };
    for (const QString& location : commonInstallLocations) {
        if (QFileInfo::exists(location)) {
            return location;
        }
    }

    return QStringLiteral("tesseract");
}
} // namespace

OcrService::OcrService()
    : processRunner_(runWithQProcess) {
}

OcrService::OcrService(ProcessRunner processRunner)
    : processRunner_(std::move(processRunner)) {
    if (!processRunner_) {
        processRunner_ = runWithQProcess;
    }
}

OcrWordResult OcrService::recognizeSingleWord(const QImage& image,
                                              const QString& tessdataDir,
                                              QString* errorMessage) const {
    OcrWordResult result;
    if (image.isNull()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Empty input image.");
        }
        return result;
    }

    QTemporaryFile inputFile(QDir::tempPath() + QStringLiteral("/wordSnapV1_ocr_XXXXXX.png"));
    inputFile.setAutoRemove(true);
    if (!inputFile.open()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to create temporary image file.");
        }
        return result;
    }

    const QString imagePath = inputFile.fileName();
    inputFile.close();

    if (!image.save(imagePath, "PNG")) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to write temporary image.");
        }
        return result;
    }

    QStringList arguments;
    arguments << imagePath << QStringLiteral("stdout") << QStringLiteral("--psm") << QStringLiteral("8")
              << QStringLiteral("-l") << QStringLiteral("eng");

    const QString trimmedTessdataDir = tessdataDir.trimmed();
    if (!trimmedTessdataDir.isEmpty()) {
        const QDir tessdata(trimmedTessdataDir);
        const QString englishModel = tessdata.filePath(QStringLiteral("eng.traineddata"));
        if (tessdata.exists() && QFileInfo::exists(englishModel)) {
            arguments << QStringLiteral("--tessdata-dir") << trimmedTessdataDir;
        }
    }

    const QString executable = resolveTesseractExecutable();
    const ProcessRunResult runResult = processRunner_(executable, arguments, 3000, 12000);
    if (!runResult.started) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Tesseract did not start. Tried: %1 | Qt: %2 | Ensure PATH contains the Tesseract folder, not tesseract.exe.")
                                .arg(executable, runResult.startError);
        }
        return result;
    }

    if (!runResult.finished) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Tesseract timed out.");
        }
        return result;
    }

    const QString stderrText = runResult.standardError.trimmed();
    const QString stdoutText = runResult.standardOutput.trimmed();

    if (!runResult.normalExit || runResult.exitCode != 0) {
        if (errorMessage != nullptr) {
            *errorMessage = stderrText.isEmpty()
                                ? QStringLiteral("Tesseract process failed.")
                                : stderrText;
        }
        return result;
    }

    if (stdoutText.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = stderrText.isEmpty()
                                ? QStringLiteral("OCR produced no text.")
                                : stderrText;
        }
        return result;
    }

    result.rawText = stdoutText;
    result.success = true;
    return result;
}
