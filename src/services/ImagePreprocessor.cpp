#include "services/ImagePreprocessor.h"

#include <QDebug>
#include <QtGlobal>

#include <array>

namespace {

struct StrategyResult {
    QImage image;
    double score = -1.0;
    double blackRatio = 0.0;
    double edgeRatio = 0.0;
    bool acceptable = false;
};

double clamp01(double value) {
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

QImage applyThreshold(const QImage& image, int threshold) {
    QImage binary(image.size(), QImage::Format_Grayscale8);
    binary.fill(255);

    for (int y = 0; y < image.height(); ++y) {
        const uchar* srcLine = image.constScanLine(y);
        uchar* dstLine = binary.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            dstLine[x] = srcLine[x] <= threshold ? 0 : 255;
        }
    }

    return binary;
}

void invertGrayscaleImage(QImage* image) {
    if (image == nullptr || image->isNull()) {
        return;
    }

    for (int y = 0; y < image->height(); ++y) {
        uchar* scanLine = image->scanLine(y);
        for (int x = 0; x < image->width(); ++x) {
            scanLine[x] = static_cast<uchar>(255 - scanLine[x]);
        }
    }
}

StrategyResult evaluateBinaryImage(const QImage& binary) {
    if (binary.isNull()) {
        return {};
    }

    int blackPixelCount = 0;
    int whitePixelCount = 0;
    int edgeTransitions = 0;

    for (int y = 0; y < binary.height(); ++y) {
        const uchar* scanLine = binary.constScanLine(y);
        for (int x = 0; x < binary.width(); ++x) {
            if (scanLine[x] == 0) {
                ++blackPixelCount;
            } else {
                ++whitePixelCount;
            }

            if (x > 0 && scanLine[x] != scanLine[x - 1]) {
                ++edgeTransitions;
            }
            if (y > 0) {
                const uchar* prevLine = binary.constScanLine(y - 1);
                if (scanLine[x] != prevLine[x]) {
                    ++edgeTransitions;
                }
            }
        }
    }

    const int totalPixels = blackPixelCount + whitePixelCount;
    if (totalPixels <= 0) {
        return {};
    }

    const double blackRatio = static_cast<double>(blackPixelCount) / static_cast<double>(totalPixels);
    const bool hasForegroundAndBackground = blackPixelCount > 0 && whitePixelCount > 0;

    const double targetBlackRatio = 0.18;
    const double normalizedDistance = qAbs(blackRatio - targetBlackRatio) / targetBlackRatio;
    const double balanceScore = 1.0 - clamp01(normalizedDistance);

    const int maxHorizontalEdges = qMax(0, binary.width() - 1) * binary.height();
    const int maxVerticalEdges = qMax(0, binary.height() - 1) * binary.width();
    const int maxEdges = maxHorizontalEdges + maxVerticalEdges;
    const double edgeRatio = maxEdges > 0 ? static_cast<double>(edgeTransitions) / static_cast<double>(maxEdges) : 0.0;
    const double normalizedEdgeScore = clamp01(edgeRatio / 0.08);

    const double separationScore = hasForegroundAndBackground ? 1.0 : 0.0;
    const double score = separationScore * 0.6 + balanceScore * 0.25 + normalizedEdgeScore * 0.15;

    const bool acceptable = hasForegroundAndBackground && blackRatio >= 0.02 && blackRatio <= 0.45;

    StrategyResult result;
    result.image = binary;
    result.score = score;
    result.blackRatio = blackRatio;
    result.edgeRatio = edgeRatio;
    result.acceptable = acceptable;
    return result;
}

} // namespace

QImage ImagePreprocessor::preprocessForWordRecognition(const QImage& source) const {
    if (source.isNull()) {
        return {};
    }

    const QImage gray = source.convertToFormat(QImage::Format_Grayscale8);
    if (gray.isNull()) {
        return {};
    }

    const std::array<PreprocessStrategy, 4> strategies = {
        PreprocessStrategy{1, 2, true, 165, false},
        PreprocessStrategy{2, 3, true, 165, false},
        PreprocessStrategy{3, 2, true, 165, true},
        PreprocessStrategy{4, 2, false, 165, false},
    };

    StrategyResult bestResult;
    int bestStrategyId = 0;

    for (int strategyIndex = 0; strategyIndex < static_cast<int>(strategies.size()); ++strategyIndex) {
        const PreprocessStrategy& strategy = strategies[strategyIndex];

        const int scaledWidth = qMax(1, gray.width() * strategy.scaleMultiplier);
        const int scaledHeight = qMax(1, gray.height() * strategy.scaleMultiplier);
        QImage transformed = gray.scaled(
            scaledWidth,
            scaledHeight,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);

        if (strategy.invertInput) {
            invertGrayscaleImage(&transformed);
        }

        int minLuminance = 255;
        int maxLuminance = 0;
        for (int y = 0; y < transformed.height(); ++y) {
            const uchar* srcLine = transformed.constScanLine(y);
            for (int x = 0; x < transformed.width(); ++x) {
                const int value = srcLine[x];
                minLuminance = qMin(minLuminance, value);
                maxLuminance = qMax(maxLuminance, value);
            }
        }

        int threshold = strategy.fixedThreshold;
        if (strategy.adaptiveThreshold && maxLuminance - minLuminance >= 10) {
            threshold = (minLuminance + maxLuminance) / 2;
        }

        StrategyResult currentResult = evaluateBinaryImage(applyThreshold(transformed, threshold));
        if (currentResult.image.isNull()) {
            continue;
        }

        if (currentResult.blackRatio > 0.5) {
            invertGrayscaleImage(&currentResult.image);
            currentResult = evaluateBinaryImage(currentResult.image);
        }

        if (currentResult.score > bestResult.score || bestResult.image.isNull()) {
            bestResult = currentResult;
            bestStrategyId = strategy.id;
        }

        if (currentResult.acceptable) {
#ifndef NDEBUG
            qDebug().nospace()
                << "[ImagePreprocessor] strategy=" << strategy.id
                << " retries=" << strategyIndex
                << " threshold=" << threshold
                << " scale=" << strategy.scaleMultiplier
                << " invertInput=" << strategy.invertInput
                << " blackRatio=" << currentResult.blackRatio
                << " edgeRatio=" << currentResult.edgeRatio;
#endif
            return currentResult.image;
        }
    }

    if (!bestResult.image.isNull()) {
#ifndef NDEBUG
        qDebug().nospace()
            << "[ImagePreprocessor] strategy=" << bestStrategyId
            << " retries=" << (strategies.size() - 1)
            << " fallback=true"
            << " blackRatio=" << bestResult.blackRatio
            << " edgeRatio=" << bestResult.edgeRatio;
#endif
        return bestResult.image;
    }

    return {};
}
