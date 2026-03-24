#pragma once

#include <QImage>
#include <QString>

#include "app/AppTypes.h"

// Wraps OCR invocation (Tesseract CLI in V1).
class OcrService final {
public:
    OcrWordResult recognizeSingleWord(const QImage& image,
                                      const QString& tessdataDir,
                                      QString* errorMessage = nullptr) const;
};
