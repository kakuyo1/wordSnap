#include "services/ImagePreprocessor.h"

#include <QtGlobal>

QImage ImagePreprocessor::preprocessForWordRecognition(const QImage& source) const {
    if (source.isNull()) {
        return {};
    }

    QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
    if (gray.isNull()) {
        return {};
    }

    const int scaledWidth = qMax(1, gray.width() * 2);
    const int scaledHeight = qMax(1, gray.height() * 2);
    gray = gray.scaled(scaledWidth, scaledHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QImage binary(gray.size(), QImage::Format_Grayscale8);
    binary.fill(255);

    for (int y = 0; y < gray.height(); ++y) {
        const uchar* srcLine = gray.constScanLine(y);
        uchar* dstLine = binary.scanLine(y);
        for (int x = 0; x < gray.width(); ++x) {
            dstLine[x] = srcLine[x] < 165 ? 0 : 255;
        }
    }

    return binary;
}
