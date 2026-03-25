#pragma once

#include <QWidget>

class QLabel;
class QPropertyAnimation;
class QTimer;

// Lightweight floating result card shown near the cursor.
class ResultCardWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ResultCardWidget(QWidget* parent = nullptr);

    void showMessage(const QString& headword,
                     const QString& body,
                     const QString& phonetic,
                     const QPoint& anchorGlobalPos,
                     int autoHideMs = 4200);

    void setCardOpacityPercent(int opacityPercent);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* headwordLabel_{nullptr};
    QLabel* phoneticLabel_{nullptr};
    QLabel* bodyLabel_{nullptr};
    qreal targetWindowOpacity_{0.92};
    QPropertyAnimation* fadeInAnimation_{nullptr};
    QPropertyAnimation* popInAnimation_{nullptr};
    QTimer* autoHideTimer_{nullptr};
};
