#include "ui/ResultCardWidget.h"

#include "app/AppTypes.h"

#include <QGuiApplication>
#include <QEasingCurve>
#include <QLabel>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

ResultCardWidget::ResultCardWidget(QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setObjectName(QStringLiteral("resultCardWidget"));

    headwordLabel_ = new QLabel(this);
    headwordLabel_->setObjectName(QStringLiteral("headwordLabel"));
    headwordLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    phoneticLabel_ = new QLabel(this);
    phoneticLabel_->setObjectName(QStringLiteral("phoneticLabel"));
    phoneticLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    phoneticLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    phoneticLabel_->hide();

    bodyLabel_ = new QLabel(this);
    bodyLabel_->setObjectName(QStringLiteral("bodyLabel"));
    bodyLabel_->setWordWrap(true);
    bodyLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    bodyLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(38, 30, 32, 26);
    root->setSpacing(8);

    auto* titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(4);
    titleLayout->addWidget(headwordLabel_);
    titleLayout->addWidget(phoneticLabel_);
    titleLayout->addStretch(1);

    root->addLayout(titleLayout);
    root->addWidget(bodyLabel_);

    setMinimumWidth(300);
    setMaximumWidth(460);

    setStyleSheet(
        QStringLiteral(
            "QLabel#headwordLabel {"
            "  font-family: 'Georgia', 'Times New Roman', 'Microsoft YaHei UI';"
            "  font-size: 20px;"
            "  font-weight: 700;"
            "  color: #4f3015;"
            "  letter-spacing: 0.5px;"
            "}"
            "QLabel#phoneticLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 12px;"
            "  font-weight: 600;"
            "  color: #8a6a4a;"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 13px;"
            "  color: #403428;"
            "}"
        )
    );

    fadeInAnimation_ = new QPropertyAnimation(this, "windowOpacity", this);
    fadeInAnimation_->setDuration(170);
    fadeInAnimation_->setEasingCurve(QEasingCurve::OutCubic);

    popInAnimation_ = new QPropertyAnimation(this, "pos", this);
    popInAnimation_->setDuration(180);
    popInAnimation_->setEasingCurve(QEasingCurve::OutBack);

    autoHideTimer_ = new QTimer(this);
    autoHideTimer_->setSingleShot(true);
    connect(autoHideTimer_, &QTimer::timeout, this, &QWidget::hide);

    setCardOpacityPercent(92);
}

void ResultCardWidget::setCardOpacityPercent(const int opacityPercent) {
    const int clamped = clampResultCardOpacityPercent(opacityPercent);
    targetWindowOpacity_ = static_cast<qreal>(clamped) / 100.0;

    if (!isVisible()) {
        return;
    }

    if (fadeInAnimation_ != nullptr) {
        fadeInAnimation_->stop();
    }
    setWindowOpacity(targetWindowOpacity_);
}

void ResultCardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF cardRect = QRectF(rect()).adjusted(10, 8, -10, -10);
    constexpr qreal kRadius = 16.0;

    for (int i = 0; i < 5; ++i) {
        const QRectF shadowRect = cardRect.adjusted(-i, -i, i, i + 2);
        const QColor shadowColor(64, 45, 28, 34 - i * 6);
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowColor);
        painter.drawRoundedRect(shadowRect, kRadius + i, kRadius + i);
    }

    QLinearGradient paperGradient(cardRect.topLeft(), cardRect.bottomLeft());
    paperGradient.setColorAt(0.0, QColor(255, 251, 242));
    paperGradient.setColorAt(0.52, QColor(249, 239, 222));
    paperGradient.setColorAt(1.0, QColor(241, 227, 203));

    painter.setPen(QPen(QColor(193, 165, 129), 1.2));
    painter.setBrush(paperGradient);
    painter.drawRoundedRect(cardRect, kRadius, kRadius);

    const QRectF innerRect = cardRect.adjusted(3, 3, -3, -3);
    painter.setPen(QPen(QColor(232, 217, 191, 200), 1.0, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(innerRect, kRadius - 3.0, kRadius - 3.0);

    painter.setPen(QPen(QColor(216, 165, 116, 120), 1.4));
    painter.drawLine(
        QPointF(cardRect.left() + 28.0, cardRect.top() + 18.0),
        QPointF(cardRect.left() + 28.0, cardRect.bottom() - 14.0));

    painter.setPen(QPen(QColor(218, 198, 168, 140), 1.0));
    const qreal lineStartX = cardRect.left() + 42.0;
    const qreal lineEndX = cardRect.right() - 18.0;
    for (qreal y = cardRect.top() + 62.0; y < cardRect.bottom() - 14.0; y += 22.0) {
        painter.drawLine(QPointF(lineStartX, y), QPointF(lineEndX, y));
    }

}

void ResultCardWidget::showMessage(const QString& headword,
                                   const QString& body,
                                   const QString& phonetic,
                                   const QPoint& anchorGlobalPos,
                                   int autoHideMs) {
    headwordLabel_->setText(headword.trimmed());

    QString formattedPhonetic = phonetic.trimmed();
    if (!formattedPhonetic.isEmpty()) {
        if (formattedPhonetic.size() >= 2
            && formattedPhonetic.startsWith('/')
            && formattedPhonetic.endsWith('/')) {
            formattedPhonetic = formattedPhonetic.mid(1, formattedPhonetic.size() - 2).trimmed();
        }

        const bool alreadyWrapped = formattedPhonetic.startsWith('[') && formattedPhonetic.endsWith(']');
        const bool containsBracketSegments = formattedPhonetic.contains('[') && formattedPhonetic.contains(']');
        if (!alreadyWrapped && !containsBracketSegments) {
            formattedPhonetic = QStringLiteral("[%1]").arg(formattedPhonetic);
        }
        phoneticLabel_->setText(formattedPhonetic);
        phoneticLabel_->show();
    } else {
        phoneticLabel_->clear();
        phoneticLabel_->hide();
    }

    const QString trimmedBody = body.trimmed();
    if (trimmedBody.isEmpty()) {
        bodyLabel_->setText(QStringLiteral("No preview available."));
    } else {
        bodyLabel_->setText(trimmedBody);
    }

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

    const QPoint popStart = target + QPoint(0, 9);
    move(popStart);

    if (fadeInAnimation_ != nullptr) {
        fadeInAnimation_->stop();
        fadeInAnimation_->setStartValue(std::max<qreal>(0.05, targetWindowOpacity_ * 0.34));
        fadeInAnimation_->setEndValue(targetWindowOpacity_);
    } else {
        setWindowOpacity(targetWindowOpacity_);
    }

    if (popInAnimation_ != nullptr) {
        popInAnimation_->stop();
        popInAnimation_->setStartValue(popStart);
        popInAnimation_->setEndValue(target);
    }

    show();
    raise();

    if (fadeInAnimation_ != nullptr) {
        fadeInAnimation_->start();
    }

    if (popInAnimation_ != nullptr) {
        popInAnimation_->start();
    }

    if (autoHideTimer_ == nullptr) {
        return;
    }

    autoHideTimer_->stop();
    if (autoHideMs > 0) {
        autoHideTimer_->start(autoHideMs);
    }
}
