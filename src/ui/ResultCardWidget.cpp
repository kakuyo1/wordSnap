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

namespace {
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
