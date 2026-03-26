#pragma once

#include <QWidget>

#include "app/AppTypes.h"

class QLabel;
class QPropertyAnimation;
class QTimer;

// Lightweight floating result card shown near the cursor.
class ResultCardWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ResultCardWidget(QWidget* parent = nullptr);

    void showMessage(const QString& statusCode,
                     const QString& headword,
                     const QString& body,
                     const QString& phonetic,
                     const QPoint& anchorGlobalPos,
                     int autoHideMs = 4200);
    void showAiLoading();
    void showAiContent(const AiAssistContent& content);
    void showAiError(const QString& message);

    void setCardOpacityPercent(int opacityPercent);
    void setCardStyle(ResultCardStyle style);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* statusLabel_{nullptr};
    QLabel* headwordLabel_{nullptr};
    QLabel* phoneticLabel_{nullptr};
    QLabel* bodyLabel_{nullptr};
    QLabel* aiLabel_{nullptr};
    ResultCardStyle cardStyle_{ResultCardStyle::KraftPaper};
    qreal targetWindowOpacity_{0.92};
    QPropertyAnimation* fadeInAnimation_{nullptr};
    QPropertyAnimation* popInAnimation_{nullptr};
    QTimer* autoHideTimer_{nullptr};
    int lastAutoHideMs_{4200};

    void showAiText(const QString& text, int autoHideMs);
    void applyTheme();
};
