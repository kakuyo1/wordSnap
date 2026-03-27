#include "ui/theme/ResultCardTheme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QPointF>

namespace {

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

} // namespace

namespace resultCardTheme {

QString aiTextColorForStyle(const ResultCardStyle style) {
    switch (style) {
    case ResultCardStyle::KraftPaper:
        return QStringLiteral("#4f3a26");
    case ResultCardStyle::Glassmorphism:
        return QStringLiteral("#ecf6ff");
    case ResultCardStyle::Terminal:
        return QStringLiteral("#92f5aa");
    }

    return QStringLiteral("#333333");
}

QString aiGridLineColorForStyle(const ResultCardStyle style) {
    switch (style) {
    case ResultCardStyle::KraftPaper:
        return QStringLiteral("rgba(164, 127, 86, 110)");
    case ResultCardStyle::Glassmorphism:
        return QStringLiteral("rgba(194, 222, 245, 128)");
    case ResultCardStyle::Terminal:
        return QStringLiteral("rgba(77, 191, 109, 165)");
    }

    return QStringLiteral("rgba(0, 0, 0, 80)");
}

QString etymologyHighlightColorForStyle(const ResultCardStyle style) {
    switch (style) {
    case ResultCardStyle::KraftPaper:
        return QStringLiteral("rgba(238, 214, 173, 190)");
    case ResultCardStyle::Glassmorphism:
        return QStringLiteral("rgba(134, 178, 221, 145)");
    case ResultCardStyle::Terminal:
        return QStringLiteral("rgba(31, 88, 44, 216)");
    }

    return QStringLiteral("rgba(210, 210, 210, 150)");
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
            "  padding: 1px 7px 2px 7px;"
            "  border-radius: 9px;"
            "  background: rgba(247, 229, 200, 190);"
            "  border: 1px solid rgba(186, 151, 113, 155);"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 13px;"
            "  color: #403428;"
            "}"
            "QLabel#aiLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 12px;"
            "  color: #4f3a26;"
            "  margin-top: 2px;"
            "  padding: 0;"
            "  background: transparent;"
            "  border: none;"
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
            "  padding: 1px 7px 2px 7px;"
            "  border-radius: 9px;"
            "  background: rgba(55, 98, 139, 140);"
            "  border: 1px solid rgba(188, 221, 248, 158);"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 13px;"
            "  color: #ecf6ff;"
            "}"
            "QLabel#aiLabel {"
            "  font-family: 'Segoe UI', 'Microsoft YaHei UI';"
            "  font-size: 12px;"
            "  color: rgba(229, 244, 255, 236);"
            "  margin-top: 2px;"
            "  padding: 0;"
            "  background: transparent;"
            "  border: none;"
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
            "  padding: 1px 7px 2px 7px;"
            "  border-radius: 3px;"
            "  background: rgba(18, 45, 29, 220);"
            "  border: 1px solid rgba(70, 177, 98, 185);"
            "}"
            "QLabel#bodyLabel {"
            "  font-family: 'Consolas', 'Cascadia Mono', 'Courier New';"
            "  font-size: 13px;"
            "  color: #9dffb0;"
            "}"
            "QLabel#aiLabel {"
            "  font-family: 'Consolas', 'Cascadia Mono', 'Courier New';"
            "  font-size: 12px;"
            "  color: #92f5aa;"
            "  margin-top: 2px;"
            "  padding: 0;"
            "  background: transparent;"
            "  border: none;"
            "}");
    }

    return QString();
}

void paintCardBackground(QPainter* painter, const QRectF& cardRect, const ResultCardStyle style) {
    if (painter == nullptr) {
        return;
    }

    switch (style) {
    case ResultCardStyle::KraftPaper:
        paintKraftPaper(painter, cardRect);
        break;
    case ResultCardStyle::Glassmorphism:
        paintGlassmorphism(painter, cardRect);
        break;
    case ResultCardStyle::Terminal:
        paintTerminal(painter, cardRect);
        break;
    }
}

} // namespace resultCardTheme
