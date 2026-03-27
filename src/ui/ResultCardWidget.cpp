#include "ui/ResultCardWidget.h"

#include "ui/theme/ResultCardTheme.h"

#include <QGuiApplication>
#include <QEasingCurve>
#include <QCursor>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPropertyAnimation>
#include <QScreen>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

namespace {
constexpr int kCardEdgeMargin = 8;
constexpr int kCardAnchorOffsetX = 14;
constexpr int kCardAnchorOffsetY = 18;
constexpr int kRepositionAnimationMs = 160;

QRect availableAreaForPoint(const QPoint& globalPos) {
    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen != nullptr) {
        return screen->availableGeometry();
    }

    return QRect(globalPos.x() - 800, globalPos.y() - 600, 1600, 1200);
}

QPoint clampTopLeftToArea(QPoint topLeft, const QSize& cardSize, const QRect& area) {
    const int minX = area.left() + kCardEdgeMargin;
    const int minY = area.top() + kCardEdgeMargin;
    const int maxX = area.right() - cardSize.width() - kCardEdgeMargin + 1;
    const int maxY = area.bottom() - cardSize.height() - kCardEdgeMargin + 1;

    if (maxX >= minX) {
        topLeft.setX(std::clamp(topLeft.x(), minX, maxX));
    } else {
        topLeft.setX(minX);
    }

    if (maxY >= minY) {
        topLeft.setY(std::clamp(topLeft.y(), minY, maxY));
    } else {
        topLeft.setY(minY);
    }

    return topLeft;
}

QPoint chooseBestCardPosition(const QPoint& anchorGlobalPos, const QSize& cardSize, const QRect& area) {
    const QPoint preferred = anchorGlobalPos + QPoint(kCardAnchorOffsetX, kCardAnchorOffsetY);
    const std::array<QPoint, 4> candidates{
        preferred,
        anchorGlobalPos + QPoint(-cardSize.width() - kCardAnchorOffsetX, kCardAnchorOffsetY),
        anchorGlobalPos + QPoint(kCardAnchorOffsetX, -cardSize.height() - kCardAnchorOffsetY),
        anchorGlobalPos + QPoint(-cardSize.width() - kCardAnchorOffsetX, -cardSize.height() - kCardAnchorOffsetY)
    };

    QPoint bestPosition = clampTopLeftToArea(candidates[0], cardSize, area);
    int bestOverflowCost = std::abs(candidates[0].x() - bestPosition.x())
                           + std::abs(candidates[0].y() - bestPosition.y());
    int bestTravelCost = std::abs(bestPosition.x() - preferred.x())
                         + std::abs(bestPosition.y() - preferred.y());

    for (size_t i = 1; i < candidates.size(); ++i) {
        const QPoint clamped = clampTopLeftToArea(candidates[i], cardSize, area);
        const int overflowCost = std::abs(candidates[i].x() - clamped.x())
                                 + std::abs(candidates[i].y() - clamped.y());
        const int travelCost = std::abs(clamped.x() - preferred.x())
                               + std::abs(clamped.y() - preferred.y());

        if (overflowCost < bestOverflowCost
            || (overflowCost == bestOverflowCost && travelCost < bestTravelCost)) {
            bestPosition = clamped;
            bestOverflowCost = overflowCost;
            bestTravelCost = travelCost;
        }
    }

    return bestPosition;
}

void moveWidgetWithBufferAnimation(QWidget* widget,
                                   QPropertyAnimation* animation,
                                   const QPoint& targetPos) {
    if (widget == nullptr) {
        return;
    }

    if (widget->pos() == targetPos) {
        return;
    }

    if (animation == nullptr || !widget->isVisible()) {
        widget->move(targetPos);
        return;
    }

    animation->stop();
    animation->setDuration(kRepositionAnimationMs);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    const QPoint startPos = widget->pos();
    const QPoint delta = targetPos - startPos;
    const QPoint settlePos(
        targetPos.x() + (delta.x() == 0 ? 0 : (delta.x() > 0 ? 4 : -4)),
        targetPos.y() + (delta.y() == 0 ? 0 : (delta.y() > 0 ? 4 : -4)));

    animation->setStartValue(startPos);
    animation->setKeyValueAt(0.82, settlePos);
    animation->setEndValue(targetPos);
    animation->start();
}

void keepWidgetInsideCurrentScreen(QWidget* widget,
                                   QPropertyAnimation* animation,
                                   const bool useBufferAnimation) {
    if (widget == nullptr) {
        return;
    }

    const QRect area = availableAreaForPoint(widget->frameGeometry().center());
    const QPoint clampedPos = clampTopLeftToArea(widget->pos(), widget->size(), area);
    if (clampedPos == widget->pos()) {
        return;
    }

    if (useBufferAnimation) {
        moveWidgetWithBufferAnimation(widget, animation, clampedPos);
    } else {
        widget->move(clampedPos);
    }
}

QString normalizedStatusCode(QString statusCode) {
    statusCode = statusCode.trimmed().toUpper();
    return statusCode.isEmpty() ? QStringLiteral("FOUND") : statusCode;
}

QString aiCellHtml(const QString& title,
                   const QString& value,
                   const bool highlighted,
                   const bool showDivider,
                   const ResultCardStyle style) {
    const QString safeValue = value.trimmed().isEmpty() ? QStringLiteral("-") : value.trimmed().toHtmlEscaped();
    QString valueHtml = safeValue;
    if (highlighted) {
        valueHtml = QStringLiteral("<span style=\"background:%1; border-radius:4px; padding:1px 3px;\">%2</span>")
                        .arg(resultCardTheme::etymologyHighlightColorForStyle(style), safeValue);
    }

    const QString divider = showDivider
                                ? QStringLiteral(" border-right:1px solid %1;").arg(resultCardTheme::aiGridLineColorForStyle(style))
                                : QString();

    return QStringLiteral(
               "<td style=\"width:33%; vertical-align:top; padding:0 6px;%3\">"
               "  <div style=\"font-size:10px; font-weight:800; letter-spacing:0.7px; text-transform:uppercase;\">%1</div>"
               "  <div style=\"margin-top:3px; font-size:11px; line-height:1.32;\">%2</div>"
               "</td>")
        .arg(title.toHtmlEscaped(), valueHtml, divider);
}

QString buildAiGridHtml(const AiAssistContent& content, const ResultCardStyle style) {
    const QString definitionCell = aiCellHtml(QStringLiteral("Definition"), content.definitionEn, false, true, style);
    const QString rootsCell = aiCellHtml(QStringLiteral("Roots"), content.roots, false, true, style);
    const QString etymologyCell = aiCellHtml(QStringLiteral("Etymology"), content.etymology, true, false, style);

    return QStringLiteral(
                "<table width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\" style=\"border-collapse:collapse; margin-top:2px; color:%1;\">"
                "  <tr>"
                "    %2"
                "    %3"
                "    %4"
                "  </tr>"
                "</table>")
        .arg(
            resultCardTheme::aiTextColorForStyle(style),
            definitionCell,
            rootsCell,
            etymologyCell);
}

QString buildLoadingDotsHtml(const int frame) {
    QString dots;
    for (int i = 0; i < 3; ++i) {
        if (i == frame % 3) {
            dots += QStringLiteral("<span style=\"font-size:16px; font-weight:800; vertical-align:super;\">.</span>");
        } else {
            dots += QStringLiteral("<span style=\"font-size:11px; opacity:0.35;\">.</span>");
        }
        if (i != 2) {
            dots += QStringLiteral("&nbsp;");
        }
    }
    return dots;
}

QString buildAiLoadingGridHtml(const int frame, const ResultCardStyle style) {
    const QString dots = buildLoadingDotsHtml(frame);
    const QString shimmerLine = QStringLiteral(
        "<div style=\"margin:3px 0 5px 0; border-top:1px solid %1;\"></div>")
                                   .arg(resultCardTheme::aiGridLineColorForStyle(style));

    return QStringLiteral(
               "%1"
               "<table width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\" style=\"border-collapse:collapse; color:%2;\">"
               "  <tr>"
               "    <td style=\"width:33%; vertical-align:top; padding:0 6px;\"><div style=\"font-size:10px; font-weight:800; letter-spacing:0.7px; text-transform:uppercase;\">Definition</div><div style=\"margin-top:4px;\">%3</div></td>"
               "    <td style=\"width:33%; vertical-align:top; padding:0 6px;\"><div style=\"font-size:10px; font-weight:800; letter-spacing:0.7px; text-transform:uppercase;\">Roots</div><div style=\"margin-top:4px;\">%3</div></td>"
               "    <td style=\"width:33%; vertical-align:top; padding:0 6px;\"><div style=\"font-size:10px; font-weight:800; letter-spacing:0.7px; text-transform:uppercase;\">Etymology</div><div style=\"margin-top:4px;\">%3</div></td>"
               "  </tr>"
               "</table>")
        .arg(shimmerLine, resultCardTheme::aiTextColorForStyle(style), dots);
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
    bodyLabel_->setTextFormat(Qt::PlainText);
    bodyLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    aiLabel_ = new QLabel(this);
    aiLabel_->setObjectName(QStringLiteral("aiLabel"));
    aiLabel_->setWordWrap(true);
    aiLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    aiLabel_->setTextFormat(Qt::RichText);
    aiLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    aiLabel_->hide();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(38, 30, 32, 26);
    root->setSpacing(5);

    auto* titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(6);
    titleLayout->addWidget(statusLabel_);
    titleLayout->addWidget(headwordLabel_);
    titleLayout->addWidget(phoneticLabel_);
    titleLayout->addStretch(1);

    root->addLayout(titleLayout);
    root->addWidget(bodyLabel_);
    root->addWidget(aiLabel_);

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
    connect(autoHideTimer_, &QTimer::timeout, this, &ResultCardWidget::onAutoHideTimeout);

    aiLoadingTimer_ = new QTimer(this);
    aiLoadingTimer_->setInterval(180);
    connect(aiLoadingTimer_, &QTimer::timeout, this, &ResultCardWidget::updateAiLoadingFrame);

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
    resultCardTheme::paintCardBackground(&painter, cardRect, cardStyle_);
}

void ResultCardWidget::enterEvent(QEnterEvent* event) {
    pendingHideOnLeave_ = false;
    QWidget::enterEvent(event);
}

void ResultCardWidget::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    if (!pendingHideOnLeave_) {
        return;
    }

    pendingHideOnLeave_ = false;
    hide();
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

    if (aiLoadingTimer_ != nullptr) {
        aiLoadingTimer_->stop();
    }
    aiLoadingFrame_ = 0;

    if (aiLabel_ != nullptr) {
        aiLabel_->clear();
        aiLabel_->hide();
    }

    adjustSize();

    const QRect area = availableAreaForPoint(anchorGlobalPos);

    const QPoint target = chooseBestCardPosition(anchorGlobalPos, size(), area);

    const int startXOffset = target.x() < anchorGlobalPos.x() ? -6 : 6;
    const int startYOffset = target.y() < anchorGlobalPos.y() ? -8 : 8;
    const QPoint popStart = target + QPoint(startXOffset, startYOffset);
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

    lastAutoHideMs_ = autoHideMs;
    pendingHideOnLeave_ = false;
    autoHideTimer_->stop();
    if (autoHideMs > 0) {
        autoHideTimer_->start(autoHideMs);
    }
}

void ResultCardWidget::showAiLoading() {
    aiLoadingFrame_ = 0;
    if (aiLoadingTimer_ != nullptr) {
        aiLoadingTimer_->start();
    }

    showAiText(buildAiLoadingGridHtml(aiLoadingFrame_, cardStyle_), std::max(lastAutoHideMs_, 9000));
}

void ResultCardWidget::showAiContent(const AiAssistContent& content) {
    const bool hasAnyContent = !content.definitionEn.trimmed().isEmpty()
                               || !content.roots.trimmed().isEmpty()
                               || !content.etymology.trimmed().isEmpty();
    if (!hasAnyContent) {
        showAiError(QStringLiteral("AI output is empty."));
        return;
    }

    if (aiLoadingTimer_ != nullptr) {
        aiLoadingTimer_->stop();
    }

    showAiText(buildAiGridHtml(content, cardStyle_), 5200);
}

void ResultCardWidget::showAiError(const QString& message) {
    if (aiLoadingTimer_ != nullptr) {
        aiLoadingTimer_->stop();
    }

    const QString trimmed = message.trimmed();
    const QString detail = trimmed.isEmpty() ? QStringLiteral("AI assist is unavailable.") : trimmed;
    const QString text = QStringLiteral(
                             "<div style=\"font-size:11px; opacity:0.85;\">AI unavailable: %1</div>")
                             .arg(detail.toHtmlEscaped());
    showAiText(text, 5000);
}

void ResultCardWidget::showAiText(const QString& text, const int autoHideMs) {
    if (aiLabel_ == nullptr) {
        return;
    }

    aiLabel_->setText(text.trimmed());
    aiLabel_->show();
    adjustSize();
    keepWidgetInsideCurrentScreen(this, popInAnimation_, true);

    if (autoHideTimer_ == nullptr) {
        return;
    }

    autoHideTimer_->stop();
    if (autoHideMs > 0) {
        autoHideTimer_->start(autoHideMs);
    }
}

void ResultCardWidget::updateAiLoadingFrame() {
    if (aiLabel_ == nullptr || !aiLabel_->isVisible()) {
        return;
    }

    ++aiLoadingFrame_;
    aiLabel_->setText(buildAiLoadingGridHtml(aiLoadingFrame_, cardStyle_));
    adjustSize();
    keepWidgetInsideCurrentScreen(this, popInAnimation_, false);
}

void ResultCardWidget::onAutoHideTimeout() {
    const QPoint localCursorPos = mapFromGlobal(QCursor::pos());
    if (rect().contains(localCursorPos)) {
        pendingHideOnLeave_ = true;
        return;
    }

    pendingHideOnLeave_ = false;
    hide();
}

void ResultCardWidget::applyTheme() {
    setStyleSheet(resultCardTheme::styleSheetForStyle(cardStyle_));
}
