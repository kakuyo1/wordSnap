#pragma once

#include <QImage>
#include <QString>
#include <QStringList>

#include <functional>

#include "app/AppTypes.h"

// Wraps OCR invocation (Tesseract CLI in V1).
class OcrService final {
public:
    struct ProcessRunResult {
        bool started{false};
        bool finished{false};
        bool normalExit{true};
        int exitCode{0};
        QString standardOutput;
        QString standardError;
        QString startError;
    };

    using ProcessRunner = std::function<ProcessRunResult(const QString& executable,
                                                         const QStringList& arguments,
                                                         int startTimeoutMs,
                                                         int finishTimeoutMs)>;

    OcrService();
    explicit OcrService(ProcessRunner processRunner);

    OcrWordResult recognizeSingleWord(const QImage& image,
                                      const QString& tessdataDir,
                                      QString* errorMessage = nullptr) const;

private:
    ProcessRunner processRunner_;
};
