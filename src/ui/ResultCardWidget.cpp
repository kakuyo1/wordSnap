#include "ui/ResultCardWidget.h"

#include <QGuiApplication>
#include <QLabel>
#include <QPoint>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

ResultCardWidget::ResultCardWidget(QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    headwordLabel_ = new QLabel(this);
    headwordLabel_->setObjectName(QStringLiteral("headwordLabel"));
    headwordLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    bodyLabel_ = new QLabel(this);
    bodyLabel_->setObjectName(QStringLiteral("bodyLabel"));
    bodyLabel_->setWordWrap(true);
    bodyLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(6);
    root->addWidget(headwordLabel_);
    root->addWidget(bodyLabel_);

    setMinimumWidth(260);
    setMaximumWidth(420);

    setStyleSheet(
        QStringLiteral(
            "QWidget {"
            "  background: #fff8f1;"
            "  border: 1px solid #d7c6b4;"
            "  border-radius: 10px;"
            "}"
            "QLabel#headwordLabel {"
            "  font-size: 17px;"
            "  font-weight: 700;"
            "  color: #4a2d16;"
            "}"
            "QLabel#bodyLabel {"
            "  font-size: 13px;"
            "  color: #2f2a25;"
            "}"
        )
    );
}

void ResultCardWidget::showMessage(const QString& headword,
                                   const QString& body,
                                   const QPoint& anchorGlobalPos,
                                   int autoHideMs) {
    headwordLabel_->setText(headword.trimmed());
    bodyLabel_->setText(body.trimmed());

    adjustSize();

    QScreen* screen = QGuiApplication::screenAt(anchorGlobalPos);
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }

    const QRect area = screen != nullptr
                           ? screen->availableGeometry()
                           : QRect(anchorGlobalPos.x() - 800, anchorGlobalPos.y() - 600, 1600, 1200);

    QPoint target = anchorGlobalPos + QPoint(14, 18);
    if (target.x() + width() > area.right() - 8) {
        target.setX(area.right() - width() - 8);
    }
    if (target.y() + height() > area.bottom() - 8) {
        target.setY(anchorGlobalPos.y() - height() - 18);
    }
    if (target.x() < area.left() + 8) {
        target.setX(area.left() + 8);
    }
    if (target.y() < area.top() + 8) {
        target.setY(area.top() + 8);
    }

    move(target);
    show();
    raise();

    if (autoHideMs > 0) {
        QTimer::singleShot(autoHideMs, this, &QWidget::hide);
    }
}
