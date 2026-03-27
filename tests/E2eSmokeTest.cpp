#include <QtTest/QtTest>

#include <QCursor>
#include <QGuiApplication>
#include <QLabel>
#include <QScreen>

#include "app/LookupCoordinator.h"
#include "support/LookupCoordinatorFixture.h"
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
} // namespace

class E2eSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void lookupResultCanRenderAndAutoHide();
};

void E2eSmokeTest::lookupResultCanRenderAndAutoHide() {
    LookupCoordinatorFixture fixture;
    LookupCoordinator coordinator = fixture.createCoordinator();

    const LookupCoordinator::Result result = coordinator.run(QRect(0, 0, 20, 20), QString());
    QCOMPARE(result.status, LookupCoordinator::Status::Found);

    ResultCardWidget widget;
    widget.showMessage(
        result.statusCode,
        result.cardTitle,
        result.cardBody,
        result.cardPhonetic,
        QCursor::pos() + QPoint(380, 260),
        140);

    QTRY_VERIFY(widget.isVisible());

    auto* statusLabel = widget.findChild<QLabel*>(QStringLiteral("statusLabel"));
    QVERIFY(statusLabel != nullptr);
    QCOMPARE(statusLabel->text(), QStringLiteral("FOUND"));

    const QRect area = availableAreaForWidget(widget);
    QVERIFY(area.isValid());
    QTRY_VERIFY(area.contains(widget.frameGeometry()));

    QPoint safeCursorPos = area.topLeft() + QPoint(6, 6);
    if (widget.frameGeometry().contains(safeCursorPos)) {
        safeCursorPos = area.bottomRight() - QPoint(6, 6);
    }
    QCursor::setPos(safeCursorPos);
    QTRY_VERIFY_WITH_TIMEOUT(!widget.frameGeometry().contains(QCursor::pos()), 300);

    QTRY_VERIFY_WITH_TIMEOUT(!widget.isVisible(), 2400);
}

QTEST_MAIN(E2eSmokeTest)

#include "E2eSmokeTest.moc"
