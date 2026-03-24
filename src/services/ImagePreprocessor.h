#pragma once

#include <QImage>

// Applies lightweight preprocessing tuned for OCR readability.
class ImagePreprocessor final {
public:
    QImage preprocessForWordRecognition(const QImage& source) const;
};
