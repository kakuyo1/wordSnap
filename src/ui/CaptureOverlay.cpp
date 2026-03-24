#include "ui/CaptureOverlay.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>

namespace {
constexpr int kMinSelectionSize = 8;
}

CaptureOverlay::CaptureOverlay(QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void CaptureOverlay::beginCapture() {
    dragging_ = false;
    capturing_ = true;
    selectionLocal_ = QRect();

    setGeometry(virtualDesktopGeometry());
    setCursor(Qt::CrossCursor);
    show();
    raise();
    activateWindow();
    grabKeyboard();
}

bool CaptureOverlay::isCapturing() const noexcept {
    return capturing_;
}

void CaptureOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 95));

    if (!selectionLocal_.isNull()) {
        const QRect normalized = selectionLocal_.normalized();

        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(normalized, Qt::transparent);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setPen(QPen(QColor(0, 194, 255), 2));
        painter.drawRect(normalized.adjusted(0, 0, -1, -1));
    }
}

void CaptureOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        finishCapture(true);
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    dragging_ = true;
    dragStartLocal_ = event->position().toPoint();
    selectionLocal_ = QRect(dragStartLocal_, QSize(1, 1));
    update();
}

void CaptureOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (!dragging_) {
        return;
    }

    selectionLocal_ = QRect(dragStartLocal_, event->position().toPoint()).normalized();
    update();
}

void CaptureOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !dragging_) {
        return;
    }

    dragging_ = false;
    const QRect selected = selectionLocal_.normalized();

    if (selected.width() < kMinSelectionSize || selected.height() < kMinSelectionSize) {
        finishCapture(true);
        return;
    }

    const QRect globalRect = selected.translated(geometry().topLeft());
    finishCapture(false);
    emit regionSelected(globalRect);
}

void CaptureOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        finishCapture(true);
        return;
    }

    QWidget::keyPressEvent(event);
}

QRect CaptureOverlay::virtualDesktopGeometry() const {
    QRect desktop;
    const QList<QScreen*> screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        if (screen != nullptr) {
            desktop = desktop.united(screen->geometry());
        }
    }
    return desktop;
}

void CaptureOverlay::finishCapture(const bool canceled) {
    dragging_ = false;
    capturing_ = false;
    releaseKeyboard();
    unsetCursor();
    hide();

    if (canceled) {
        emit captureCanceled();
    }
}
