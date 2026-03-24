#pragma once

#include <QWidget>

class QLabel;

// Lightweight floating result card shown near the cursor.
class ResultCardWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ResultCardWidget(QWidget* parent = nullptr);

    void showMessage(const QString& headword,
                     const QString& body,
                     const QPoint& anchorGlobalPos,
                     int autoHideMs = 4200);

private:
    QLabel* headwordLabel_{nullptr};
    QLabel* bodyLabel_{nullptr};
};
