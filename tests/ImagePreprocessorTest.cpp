#include <QtTest/QtTest>

#include "services/ImagePreprocessor.h"

class ImagePreprocessorTest : public QObject {
    Q_OBJECT

private slots:
    void preprocessReturnsNullForNullImage();
    void preprocessUpscalesAndKeepsLowContrastText();
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

QTEST_APPLESS_MAIN(ImagePreprocessorTest)

#include "ImagePreprocessorTest.moc"
