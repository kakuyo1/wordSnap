#pragma once

#include <QImage>
#include <QRect>
#include <QString>

#include <functional>

#include "app/AppTypes.h"

class LookupCoordinator final {
public:
    enum class Status {
        Found,
        OcrFailed,
        Unknown,
        DictUnavailable
    };

    struct Dependencies {
        std::function<QImage(const QRect&)> capture;
        std::function<QImage(const QImage&)> preprocess;
        std::function<OcrWordResult(const QImage&, const QString&, QString*)> recognizeSingleWord;
        std::function<QString(const QString&)> normalizeCandidate;
        std::function<bool()> isDictionaryReady;
        std::function<DictionaryEntry(const QString&)> lookup;
    };

    struct Result {
        Status status{Status::OcrFailed};
        QString statusCode;
        QString queryWord;
        QString tooltipText;
        QString cardTitle;
        QString cardBody;
        QString cardPhonetic;
        QString trayMessage;
        int cardTimeoutMs{0};
        int trayTimeoutMs{0};
    };

    explicit LookupCoordinator(Dependencies dependencies);

    Result run(const QRect& globalRect,
               const QString& tessdataDir) const;

private:
    Dependencies dependencies_;
};
