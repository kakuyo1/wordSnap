#include <QtTest/QtTest>

#include <QCursor>
#include <QLabel>

#include "ui/ResultCardWidget.h"

class ResultCardWidgetTest : public QObject {
    Q_OBJECT

private slots:
    void showMessageRendersBasicFields();
    void showMessageAutoHideEventuallyHidesWidget();
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
    QTRY_VERIFY_WITH_TIMEOUT(!widget.isVisible(), 2000);
}

QTEST_MAIN(ResultCardWidgetTest)

#include "ResultCardWidgetTest.moc"
