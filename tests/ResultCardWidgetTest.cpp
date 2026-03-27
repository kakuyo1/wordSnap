#include <QtTest/QtTest>

#include <QCursor>
#include <QGuiApplication>
#include <QLabel>
#include <QScreen>

#include "ui/ResultCardWidget.h"

namespace {
QRect availableAreaForWidget(const QWidget& widget) {
    QScreen* screen = QGuiApplication::screenAt(widget.frameGeometry().center());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen == nullptr) {
        return QRect();
    }

    return screen->availableGeometry();
}

void waitForCardAnimationToSettle() {
    QTest::qWait(260);
}
} // namespace

class ResultCardWidgetTest : public QObject {
    Q_OBJECT

private slots:
    void showMessageRendersBasicFields();
    void showMessageAutoHideEventuallyHidesWidget();
    void showMessageClampsCardInsideAvailableArea();
    void showAiContentRepositionsCardInsideAvailableArea();
    void showAiLoadingKeepsStableGeometryAwayFromEdges();
};

void ResultCardWidgetTest::showMessageRendersBasicFields() {
    ResultCardWidget widget;

    widget.showMessage(
        QStringLiteral(" found "),
        QStringLiteral("  run  "),
        QStringLiteral(" to move fast "),
        QStringLiteral("/r\u028cn/"),
        QCursor::pos() + QPoint(240, 180),
        0);

    QTRY_VERIFY(widget.isVisible());

    auto* statusLabel = widget.findChild<QLabel*>(QStringLiteral("statusLabel"));
    auto* headwordLabel = widget.findChild<QLabel*>(QStringLiteral("headwordLabel"));
    auto* phoneticLabel = widget.findChild<QLabel*>(QStringLiteral("phoneticLabel"));
    auto* bodyLabel = widget.findChild<QLabel*>(QStringLiteral("bodyLabel"));
    QVERIFY(statusLabel != nullptr);
    QVERIFY(headwordLabel != nullptr);
    QVERIFY(phoneticLabel != nullptr);
    QVERIFY(bodyLabel != nullptr);

    QCOMPARE(statusLabel->text(), QStringLiteral("FOUND"));
    QCOMPARE(headwordLabel->text(), QStringLiteral("run"));
    QCOMPARE(phoneticLabel->text(), QStringLiteral("[r\u028cn]"));
    QCOMPARE(bodyLabel->text(), QStringLiteral("to move fast"));

    widget.hide();
    QVERIFY(!widget.isVisible());
}

void ResultCardWidgetTest::showMessageAutoHideEventuallyHidesWidget() {
    ResultCardWidget widget;

    widget.showMessage(
        QStringLiteral("FOUND"),
        QStringLiteral("run"),
        QStringLiteral("to move fast"),
        QStringLiteral("[r\u028cn]"),
        QCursor::pos() + QPoint(300, 220),
        120);

    QTRY_VERIFY(widget.isVisible());
    waitForCardAnimationToSettle();

    const QRect area = availableAreaForWidget(widget);
    QVERIFY(area.isValid());

    QPoint safeCursorPos = area.topLeft() + QPoint(4, 4);
    if (widget.frameGeometry().contains(safeCursorPos)) {
        safeCursorPos = area.bottomRight() - QPoint(4, 4);
    }
    QCursor::setPos(safeCursorPos);

    QTRY_VERIFY_WITH_TIMEOUT(!widget.frameGeometry().contains(QCursor::pos()), 300);
    QTRY_VERIFY_WITH_TIMEOUT(!widget.isVisible(), 2400);
}

void ResultCardWidgetTest::showMessageClampsCardInsideAvailableArea() {
    ResultCardWidget widget;

    QScreen* primary = QGuiApplication::primaryScreen();
    QVERIFY(primary != nullptr);
    const QRect primaryArea = primary->availableGeometry();

    widget.showMessage(
        QStringLiteral("FOUND"),
        QStringLiteral("run"),
        QStringLiteral("to move fast"),
        QStringLiteral("[r\u028cn]"),
        primaryArea.bottomRight() + QPoint(400, 300),
        0);

    QTRY_VERIFY(widget.isVisible());
    waitForCardAnimationToSettle();

    const QRect area = availableAreaForWidget(widget);
    QVERIFY(area.isValid());
    QTRY_VERIFY(area.contains(widget.frameGeometry()));
}

void ResultCardWidgetTest::showAiContentRepositionsCardInsideAvailableArea() {
    ResultCardWidget widget;

    QScreen* primary = QGuiApplication::primaryScreen();
    QVERIFY(primary != nullptr);
    const QRect primaryArea = primary->availableGeometry();

    widget.showMessage(
        QStringLiteral("FOUND"),
        QStringLiteral("run"),
        QStringLiteral("short"),
        QStringLiteral("[r\u028cn]"),
        primaryArea.bottomRight() + QPoint(420, 320),
        0);

    QTRY_VERIFY(widget.isVisible());

    const int initialHeight = widget.frameGeometry().height();

    AiAssistContent content;
    content.definitionEn = QStringLiteral("a long explanation that forces the card to layout wrapped text");
    content.roots = QStringLiteral("from old forms with several connected morphemes");
    content.etymology = QStringLiteral("historical origin details that expand the bottom section");
    widget.showAiContent(content);

    auto* aiLabel = widget.findChild<QLabel*>(QStringLiteral("aiLabel"));
    QVERIFY(aiLabel != nullptr);
    QTRY_VERIFY(aiLabel->isVisible());
    QTRY_VERIFY(widget.frameGeometry().height() >= initialHeight);
    waitForCardAnimationToSettle();

    const QRect area = availableAreaForWidget(widget);
    QVERIFY(area.isValid());
    QTRY_VERIFY(area.contains(widget.frameGeometry()));
}

void ResultCardWidgetTest::showAiLoadingKeepsStableGeometryAwayFromEdges() {
    ResultCardWidget widget;

    QScreen* primary = QGuiApplication::primaryScreen();
    QVERIFY(primary != nullptr);
    const QRect primaryArea = primary->availableGeometry();
    const QPoint anchor = primaryArea.center();

    widget.showMessage(
        QStringLiteral("FOUND"),
        QStringLiteral("run"),
        QStringLiteral("to move fast"),
        QStringLiteral("[r\u028cn]"),
        anchor,
        0);

    QTRY_VERIFY(widget.isVisible());
    waitForCardAnimationToSettle();

    const QRect safeArea = primaryArea.adjusted(80, 80, -80, -80);
    QVERIFY(safeArea.contains(widget.frameGeometry()));

    widget.showAiLoading();

    auto* aiLabel = widget.findChild<QLabel*>(QStringLiteral("aiLabel"));
    QVERIFY(aiLabel != nullptr);
    QTRY_VERIFY(aiLabel->isVisible());
    waitForCardAnimationToSettle();

    const QRect initialGeometry = widget.frameGeometry();
    QTest::qWait(680);
    const QRect finalGeometry = widget.frameGeometry();

    QCOMPARE(finalGeometry.topLeft(), initialGeometry.topLeft());
    QCOMPARE(finalGeometry.size(), initialGeometry.size());
}

QTEST_MAIN(ResultCardWidgetTest)

#include "ResultCardWidgetTest.moc"
