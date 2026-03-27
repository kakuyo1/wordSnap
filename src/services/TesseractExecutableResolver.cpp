#include "services/TesseractExecutableResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <utility>

namespace {
QString resolveEntryCandidate(const QString& rawEntry,
                             const TesseractExecutableResolver::PathExistsChecker& pathExistsChecker) {
    const QString entry = rawEntry.trimmed();
    if (entry.isEmpty()) {
        return {};
    }

    const QFileInfo entryInfo(entry);
    if (entryInfo.fileName().compare(QStringLiteral("tesseract.exe"), Qt::CaseInsensitive) == 0
        && pathExistsChecker(entry)) {
        return entry;
    }

    const QString nestedCandidate = QDir(entry).filePath(QStringLiteral("tesseract.exe"));
    if (pathExistsChecker(nestedCandidate)) {
        return nestedCandidate;
    }

    return {};
}
} // namespace

TesseractExecutableResolver::TesseractExecutableResolver()
    : pathExistsChecker_(pathExistsOnDisk) {
}

TesseractExecutableResolver::TesseractExecutableResolver(PathExistsChecker pathExistsChecker)
    : pathExistsChecker_(std::move(pathExistsChecker)) {
    if (!pathExistsChecker_) {
        pathExistsChecker_ = pathExistsOnDisk;
    }
}

QString TesseractExecutableResolver::resolve(const RuntimeSnapshot& snapshot) const {
    const QString appLocal = QDir(snapshot.appDirPath).filePath(QStringLiteral("tesseract.exe"));
    if (pathExistsChecker_(appLocal)) {
        return appLocal;
    }

    const QString envExe = resolveEntryCandidate(snapshot.tesseractExeEnv, pathExistsChecker_);
    if (!envExe.isEmpty()) {
        return envExe;
    }

    const QString envPath = resolveEntryCandidate(snapshot.tesseractPathEnv, pathExistsChecker_);
    if (!envPath.isEmpty()) {
        return envPath;
    }

    const QStringList pathEntries = snapshot.systemPath.split(';', Qt::SkipEmptyParts);
    for (const QString& pathEntry : pathEntries) {
        const QString candidate = resolveEntryCandidate(pathEntry, pathExistsChecker_);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }

    const QString discoveredTesseract = snapshot.discoveredTesseract.trimmed();
    if (!discoveredTesseract.isEmpty()) {
        return discoveredTesseract;
    }

    const QString discoveredTesseractExe = snapshot.discoveredTesseractExe.trimmed();
    if (!discoveredTesseractExe.isEmpty()) {
        return discoveredTesseractExe;
    }

    const QStringList commonInstallLocations{
        QStringLiteral("C:/Program Files/Tesseract-OCR/tesseract.exe"),
        QStringLiteral("C:/Program Files (x86)/Tesseract-OCR/tesseract.exe")
    };
    for (const QString& location : commonInstallLocations) {
        if (pathExistsChecker_(location)) {
            return location;
        }
    }

    return QStringLiteral("tesseract");
}

TesseractExecutableResolver::RuntimeSnapshot TesseractExecutableResolver::captureRuntime() {
    RuntimeSnapshot snapshot;
    snapshot.appDirPath = QCoreApplication::applicationDirPath();
    snapshot.tesseractExeEnv = qEnvironmentVariable("TESSERACT_EXE");
    snapshot.tesseractPathEnv = qEnvironmentVariable("TESSERACT_PATH");

    const QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    snapshot.systemPath = processEnvironment.value(QStringLiteral("PATH"));

    snapshot.discoveredTesseract = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
    snapshot.discoveredTesseractExe = QStandardPaths::findExecutable(QStringLiteral("tesseract.exe"));
    return snapshot;
}

bool TesseractExecutableResolver::pathExistsOnDisk(const QString& path) {
    return QFileInfo::exists(path);
}
