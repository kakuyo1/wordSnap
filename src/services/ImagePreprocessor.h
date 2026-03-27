#pragma once

#include <QImage>

// Applies lightweight preprocessing tuned for OCR readability.
class ImagePreprocessor final {
public:
    QImage preprocessForWordRecognition(const QImage& source) const;

private:
    struct PreprocessStrategy {
        int id = 0;
        int scaleMultiplier = 2;
        bool adaptiveThreshold = true;
        int fixedThreshold = 165;
        bool invertInput = false;
    };
};
