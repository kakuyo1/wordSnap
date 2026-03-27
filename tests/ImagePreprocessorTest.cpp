#include <QtTest/QtTest>

#include "services/ImagePreprocessor.h"

class ImagePreprocessorTest : public QObject {
    Q_OBJECT

private slots:
    void preprocessReturnsNullForNullImage();
    void preprocessUpscalesAndKeepsLowContrastText();
    void preprocessNormalizesLightTextOnDarkBackground();
    void preprocessReturnsBinaryPixelsOnly();
};

void ImagePreprocessorTest::preprocessReturnsNullForNullImage() {
    const ImagePreprocessor preprocessor;
    const QImage output = preprocessor.preprocessForWordRecognition(QImage());
    QVERIFY(output.isNull());
}

void ImagePreprocessorTest::preprocessUpscalesAndKeepsLowContrastText() {
    QImage source(40, 20, QImage::Format_Grayscale8);
    source.fill(210);
    for (int y = 6; y < 14; ++y) {
        uchar* scanLine = source.scanLine(y);
        for (int x = 12; x < 28; ++x) {
            scanLine[x] = 180;
        }
    }

    const ImagePreprocessor preprocessor;
    const QImage output = preprocessor.preprocessForWordRecognition(source);

    QCOMPARE(output.width(), 80);
    QCOMPARE(output.height(), 40);

    int blackPixelCount = 0;
    int whitePixelCount = 0;
    for (int y = 0; y < output.height(); ++y) {
        const uchar* scanLine = output.constScanLine(y);
        for (int x = 0; x < output.width(); ++x) {
            if (scanLine[x] == 0) {
                ++blackPixelCount;
            }
            if (scanLine[x] == 255) {
                ++whitePixelCount;
            }
        }
    }

    QVERIFY2(blackPixelCount > 0, "Low-contrast text should survive binarization.");
    QVERIFY2(whitePixelCount > 0, "Background should stay distinguishable after binarization.");
}

void ImagePreprocessorTest::preprocessNormalizesLightTextOnDarkBackground() {
    QImage source(24, 16, QImage::Format_Grayscale8);
    source.fill(35);
    for (int y = 4; y < 12; ++y) {
        uchar* scanLine = source.scanLine(y);
        for (int x = 8; x < 16; ++x) {
            scanLine[x] = 215;
        }
    }

    const ImagePreprocessor preprocessor;
    const QImage output = preprocessor.preprocessForWordRecognition(source);

    int blackPixelCount = 0;
    int whitePixelCount = 0;
    for (int y = 0; y < output.height(); ++y) {
        const uchar* scanLine = output.constScanLine(y);
        for (int x = 0; x < output.width(); ++x) {
            if (scanLine[x] == 0) {
                ++blackPixelCount;
            }
            if (scanLine[x] == 255) {
                ++whitePixelCount;
            }
        }
    }

    QVERIFY2(whitePixelCount > blackPixelCount, "Result should prefer white background for OCR readability.");
    QCOMPARE(output.pixelColor(output.width() / 2, output.height() / 2).red(), 0);
}

void ImagePreprocessorTest::preprocessReturnsBinaryPixelsOnly() {
    QImage source(30, 10, QImage::Format_Grayscale8);
    for (int y = 0; y < source.height(); ++y) {
        uchar* scanLine = source.scanLine(y);
        for (int x = 0; x < source.width(); ++x) {
            scanLine[x] = static_cast<uchar>((x * 9 + y * 5) % 256);
        }
    }

    const ImagePreprocessor preprocessor;
    const QImage output = preprocessor.preprocessForWordRecognition(source);

    QVERIFY(!output.isNull());
    for (int y = 0; y < output.height(); ++y) {
        const uchar* scanLine = output.constScanLine(y);
        for (int x = 0; x < output.width(); ++x) {
            QVERIFY(scanLine[x] == 0 || scanLine[x] == 255);
        }
    }
}

QTEST_APPLESS_MAIN(ImagePreprocessorTest)

#include "ImagePreprocessorTest.moc"
