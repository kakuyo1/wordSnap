#include "services/OcrService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace {
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

    QProcess process;
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
    process.start(executable, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted(3000)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Tesseract did not start. Tried: %1 | Qt: %2 | Ensure PATH contains the Tesseract folder, not tesseract.exe.")
                                .arg(executable, process.errorString());
        }
        return result;
    }

    if (!process.waitForFinished(12000)) {
        process.kill();
        process.waitForFinished(1000);
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Tesseract timed out.");
        }
        return result;
    }

    const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
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
