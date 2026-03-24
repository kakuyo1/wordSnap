#pragma once

#include <QImage>
#include <QRect>

// Captures selected screen region into a QImage.
// V1 captures from the screen containing the selection center point.
class ScreenCaptureService final {
public:
    QImage capture(const QRect& globalRect) const;
};
