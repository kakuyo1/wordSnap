#pragma once

#include <QPoint>
#include <QRect>
#include <QWidget>

// Fullscreen translucent overlay for selecting a rectangular screen region.
// This widget is UI-only and emits selected geometry in global coordinates.
class CaptureOverlay final : public QWidget {
    Q_OBJECT

public:
    explicit CaptureOverlay(QWidget* parent = nullptr);

    // Shows overlay and enters rectangle selection mode.
    void beginCapture();

    bool isCapturing() const noexcept;

signals:
    void regionSelected(const QRect& globalRect);
    void captureCanceled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QRect virtualDesktopGeometry() const;
    void finishCapture(bool canceled);

private:
    bool dragging_{false};
    bool capturing_{false};
    QPoint dragStartLocal_;
    QRect selectionLocal_;
};
