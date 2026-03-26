#include "ui/ResultCardWidget.h"

#include <QGuiApplication>
#include <QEasingCurve>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QString normalizedStatusCode(QString statusCode) {
    statusCode = statusCode.trimmed().toUpper();
    return statusCode.isEmpty() ? QStringLiteral("FOUND") : statusCode;
}

QString styleSheetForStyle(const ResultCardStyle style) {
    switch (style) {
    case ResultCardStyle::KraftPaper:
        return QStringLiteral(
            "QLabel#statusLabel {"
            "  padding: 3px 8px;"
            "  border-radius: 10px;"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 10px;"
            "  font-weight: 700;"
            "  color: #6e4a2a;"
            "  background: rgba(243, 219, 181, 170);"
            "}"
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
            "}");
    case ResultCardStyle::Glassmorphism:
        return QStringLiteral(
            "QLabel#statusLabel {"
            "  padding: 3px 8px;"
            "  border-radius: 10px;"
            "  font-family: 'Segoe UI Semibold', 'Microsoft YaHei UI';"
            "  font-size: 10px;"
            "  color: rgba(250, 254, 255, 230);"
            "  background: rgba(45, 88, 126, 145);"
            "}"
            "QLabel#headwordLabel {"
            "  font-family: 'Cambria', 'Georgia', 'Microsoft YaHei UI';"
            "  font-size: 20px;"
            "  font-weight: 700;"
            "  color: #f2f9ff;"
            "}"
            "QLabel#phoneticLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 12px;"
            "  font-weight: 600;"
            "  color: rgba(226, 239, 255, 230);"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 13px;"
            "  color: #ecf6ff;"
            "}");
    case ResultCardStyle::Terminal:
        return QStringLiteral(
            "QLabel#statusLabel {"
            "  padding: 2px 8px;"
            "  border-radius: 2px;"
            "  font-family: 'Consolas', 'Cascadia Mono', 'Courier New';"
            "  font-size: 10px;"
            "  font-weight: 700;"
            "  color: #0f1a12;"
            "  background: #7ef7a0;"
            "}"
            "QLabel#headwordLabel {"
            "  font-family: 'Consolas', 'Cascadia Mono', 'Courier New';"
            "  font-size: 19px;"
            "  font-weight: 700;"
            "  color: #89ff9e;"
            "}"
            "QLabel#phoneticLabel {"
            "  font-family: 'Consolas', 'Cascadia Mono', 'Courier New';"
            "  font-size: 12px;"
            "  color: #66d684;"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Consolas', 'Cascadia Mono', 'Courier New';"
            "  font-size: 13px;"
            "  color: #9dffb0;"
            "}");
    case ResultCardStyle::Clay:
        return QStringLiteral(
            "QLabel#statusLabel {"
            "  padding: 3px 9px;"
            "  border-radius: 11px;"
            "  font-family: 'Segoe UI Semibold', 'Microsoft YaHei UI';"
            "  font-size: 10px;"
            "  color: #7f3e37;"
            "  background: rgba(255, 215, 208, 230);"
            "}"
            "QLabel#headwordLabel {"
            "  font-family: 'Trebuchet MS', 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 21px;"
            "  font-weight: 800;"
            "  color: #7c4138;"
            "}"
            "QLabel#phoneticLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 12px;"
            "  font-weight: 600;"
            "  color: #9f5f57;"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 13px;"
            "  color: #6d4b45;"
            "}");
    }

    return QString();
}

void paintKraftPaper(QPainter* painter, const QRectF& cardRect) {
    constexpr qreal kRadius = 16.0;

    for (int i = 0; i < 5; ++i) {
        const QRectF shadowRect = cardRect.adjusted(-i, -i, i, i + 2);
        const QColor shadowColor(64, 45, 28, 34 - i * 6);
        painter->setPen(Qt::NoPen);
        painter->setBrush(shadowColor);
        painter->drawRoundedRect(shadowRect, kRadius + i, kRadius + i);
    }

    QLinearGradient paperGradient(cardRect.topLeft(), cardRect.bottomLeft());
    paperGradient.setColorAt(0.0, QColor(255, 251, 242));
    paperGradient.setColorAt(0.52, QColor(249, 239, 222));
    paperGradient.setColorAt(1.0, QColor(241, 227, 203));

    painter->setPen(QPen(QColor(193, 165, 129), 1.2));
    painter->setBrush(paperGradient);
    painter->drawRoundedRect(cardRect, kRadius, kRadius);

    const QRectF innerRect = cardRect.adjusted(3, 3, -3, -3);
    painter->setPen(QPen(QColor(232, 217, 191, 200), 1.0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(innerRect, kRadius - 3.0, kRadius - 3.0);

    painter->setPen(QPen(QColor(216, 165, 116, 120), 1.4));
    painter->drawLine(
        QPointF(cardRect.left() + 28.0, cardRect.top() + 18.0),
        QPointF(cardRect.left() + 28.0, cardRect.bottom() - 14.0));

    painter->setPen(QPen(QColor(218, 198, 168, 140), 1.0));
    const qreal lineStartX = cardRect.left() + 42.0;
    const qreal lineEndX = cardRect.right() - 18.0;
    for (qreal y = cardRect.top() + 62.0; y < cardRect.bottom() - 14.0; y += 22.0) {
        painter->drawLine(QPointF(lineStartX, y), QPointF(lineEndX, y));
    }
}

void paintGlassmorphism(QPainter* painter, const QRectF& cardRect) {
    constexpr qreal kRadius = 18.0;

    for (int i = 0; i < 6; ++i) {
        const QRectF shadowRect = cardRect.adjusted(-i * 0.8, -i * 0.6, i * 1.2, i * 1.4 + 2.0);
        const QColor shadowColor(20, 34, 52, 38 - i * 5);
        painter->setPen(Qt::NoPen);
        painter->setBrush(shadowColor);
        painter->drawRoundedRect(shadowRect, kRadius + i * 0.6, kRadius + i * 0.6);
    }

    QLinearGradient outerGradient(cardRect.topLeft(), cardRect.bottomRight());
    outerGradient.setColorAt(0.0, QColor(229, 248, 255, 162));
    outerGradient.setColorAt(0.5, QColor(148, 188, 223, 124));
    outerGradient.setColorAt(1.0, QColor(87, 135, 183, 170));

    painter->setPen(QPen(QColor(231, 247, 255, 195), 1.4));
    painter->setBrush(outerGradient);
    painter->drawRoundedRect(cardRect, kRadius, kRadius);

    const QRectF shineRect = cardRect.adjusted(5.0, 4.0, -6.0, -cardRect.height() * 0.52);
    QLinearGradient shineGradient(shineRect.topLeft(), shineRect.bottomLeft());
    shineGradient.setColorAt(0.0, QColor(255, 255, 255, 185));
    shineGradient.setColorAt(1.0, QColor(255, 255, 255, 20));
    painter->setPen(Qt::NoPen);
    painter->setBrush(shineGradient);
    painter->drawRoundedRect(shineRect, 12.0, 12.0);

    painter->setPen(QPen(QColor(232, 247, 255, 118), 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(cardRect.adjusted(2.5, 2.5, -2.5, -2.5), kRadius - 2.5, kRadius - 2.5);
}

void paintTerminal(QPainter* painter, const QRectF& cardRect) {
    constexpr qreal kRadius = 8.0;

    for (int i = 0; i < 4; ++i) {
        const QRectF shadowRect = cardRect.adjusted(-i, -i, i, i + 2);
        const QColor shadowColor(0, 0, 0, 46 - i * 9);
        painter->setPen(Qt::NoPen);
        painter->setBrush(shadowColor);
        painter->drawRoundedRect(shadowRect, kRadius, kRadius);
    }

    painter->setPen(QPen(QColor(64, 209, 102, 210), 1.5));
    painter->setBrush(QColor(9, 20, 14, 232));
    painter->drawRoundedRect(cardRect, kRadius, kRadius);

    painter->setPen(QPen(QColor(35, 86, 48, 150), 1.0));
    for (qreal x = cardRect.left() + 22.0; x < cardRect.right(); x += 22.0) {
        painter->drawLine(QPointF(x, cardRect.top() + 1.0), QPointF(x, cardRect.bottom() - 1.0));
    }
    for (qreal y = cardRect.top() + 20.0; y < cardRect.bottom(); y += 20.0) {
        painter->drawLine(QPointF(cardRect.left() + 1.0, y), QPointF(cardRect.right() - 1.0, y));
    }
}

void paintClay(QPainter* painter, const QRectF& cardRect) {
    constexpr qreal kRadius = 20.0;

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(173, 123, 116, 55));
    painter->drawRoundedRect(cardRect.adjusted(-2.0, 5.0, 5.0, 9.0), kRadius + 2.0, kRadius + 2.0);

    QLinearGradient baseGradient(cardRect.topLeft(), cardRect.bottomLeft());
    baseGradient.setColorAt(0.0, QColor(255, 236, 229));
    baseGradient.setColorAt(0.56, QColor(251, 220, 210));
    baseGradient.setColorAt(1.0, QColor(242, 198, 186));

    painter->setPen(QPen(QColor(237, 186, 175, 220), 1.1));
    painter->setBrush(baseGradient);
    painter->drawRoundedRect(cardRect, kRadius, kRadius);

    const QRectF highlightRect = cardRect.adjusted(7.0, 6.0, -8.0, -cardRect.height() * 0.45);
    QLinearGradient highlightGradient(highlightRect.topLeft(), highlightRect.bottomLeft());
    highlightGradient.setColorAt(0.0, QColor(255, 252, 250, 165));
    highlightGradient.setColorAt(1.0, QColor(255, 252, 250, 8));
    painter->setPen(Qt::NoPen);
    painter->setBrush(highlightGradient);
    painter->drawRoundedRect(highlightRect, 14.0, 14.0);

    painter->setPen(QPen(QColor(221, 158, 146, 180), 1.2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(cardRect.adjusted(2.5, 2.5, -2.5, -2.5), kRadius - 2.0, kRadius - 2.0);
}
} // namespace

ResultCardWidget::ResultCardWidget(QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setObjectName(QStringLiteral("resultCardWidget"));

    statusLabel_ = new QLabel(this);
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    statusLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

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
    titleLayout->setSpacing(6);
    titleLayout->addWidget(statusLabel_);
    titleLayout->addWidget(headwordLabel_);
    titleLayout->addWidget(phoneticLabel_);
    titleLayout->addStretch(1);

    root->addLayout(titleLayout);
    root->addWidget(bodyLabel_);

    setMinimumWidth(320);
    setMaximumWidth(500);

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
    applyTheme();
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

void ResultCardWidget::setCardStyle(const ResultCardStyle style) {
    if (cardStyle_ == style) {
        return;
    }

    cardStyle_ = style;
    applyTheme();
    update();
}

void ResultCardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF cardRect = QRectF(rect()).adjusted(10, 8, -10, -10);
    switch (cardStyle_) {
    case ResultCardStyle::KraftPaper:
        paintKraftPaper(&painter, cardRect);
        break;
    case ResultCardStyle::Glassmorphism:
        paintGlassmorphism(&painter, cardRect);
        break;
    case ResultCardStyle::Terminal:
        paintTerminal(&painter, cardRect);
        break;
    case ResultCardStyle::Clay:
        paintClay(&painter, cardRect);
        break;
    }
}

void ResultCardWidget::showMessage(const QString& statusCode,
                                   const QString& headword,
                                   const QString& body,
                                   const QString& phonetic,
                                   const QPoint& anchorGlobalPos,
                                   int autoHideMs) {
    statusLabel_->setText(normalizedStatusCode(statusCode));

    const QString trimmedHeadword = headword.trimmed();
    if (trimmedHeadword.isEmpty()) {
        headwordLabel_->clear();
        headwordLabel_->hide();
    } else {
        headwordLabel_->setText(trimmedHeadword);
        headwordLabel_->show();
    }

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
        bodyLabel_->clear();
        bodyLabel_->hide();
    } else {
        bodyLabel_->setText(trimmedBody);
        bodyLabel_->show();
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

void ResultCardWidget::applyTheme() {
    setStyleSheet(styleSheetForStyle(cardStyle_));
}
