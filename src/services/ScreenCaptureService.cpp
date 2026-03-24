#include "services/ScreenCaptureService.h"

#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>

QImage ScreenCaptureService::capture(const QRect& globalRect) const {
    const QRect normalized = globalRect.normalized();
    if (normalized.isEmpty()) {
        return {};
    }

    QScreen* screen = QGuiApplication::screenAt(normalized.center());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return {};
    }

    const QRect screenRect = screen->geometry();
    const QRect clippedGlobal = normalized.intersected(screenRect);
    if (clippedGlobal.isEmpty()) {
        return {};
    }

    const QRect localRect = clippedGlobal.translated(-screenRect.topLeft());
    const QPixmap pixmap = screen->grabWindow(0, localRect.x(), localRect.y(), localRect.width(), localRect.height());
    if (pixmap.isNull()) {
        return {};
    }

    return pixmap.toImage();
}
