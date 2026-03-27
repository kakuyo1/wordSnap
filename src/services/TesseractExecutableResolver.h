#pragma once

#include <QString>

#include <functional>

class TesseractExecutableResolver final {
public:
    struct RuntimeSnapshot {
        QString appDirPath;
        QString tesseractExeEnv;
        QString tesseractPathEnv;
        QString systemPath;
        QString discoveredTesseract;
        QString discoveredTesseractExe;
    };

    using PathExistsChecker = std::function<bool(const QString& path)>;

    TesseractExecutableResolver();
    explicit TesseractExecutableResolver(PathExistsChecker pathExistsChecker);

    QString resolve(const RuntimeSnapshot& snapshot) const;
    static RuntimeSnapshot captureRuntime();

private:
    static bool pathExistsOnDisk(const QString& path);

    PathExistsChecker pathExistsChecker_;
};
