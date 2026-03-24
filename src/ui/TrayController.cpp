#include "ui/TrayController.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QStyle>
#include <QSystemTrayIcon>

TrayController::TrayController(QObject* parent)
    : QObject(parent),
      trayIcon_(std::make_unique<QSystemTrayIcon>(this)),
      trayMenu_(std::make_unique<QMenu>()),
      captureAction_(trayMenu_->addAction(QStringLiteral("Capture Now"))),
      settingsAction_(trayMenu_->addAction(QStringLiteral("Settings"))),
      quitAction_(trayMenu_->addAction(QStringLiteral("Exit"))) {
    QIcon trayIcon = QIcon::fromTheme(QStringLiteral("accessories-dictionary"));
    if (trayIcon.isNull()) {
        trayIcon = QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView);
    }
    trayIcon_->setIcon(trayIcon);
    trayIcon_->setContextMenu(trayMenu_.get());

    connect(captureAction_, &QAction::triggered, this, &TrayController::captureRequested);
    connect(settingsAction_, &QAction::triggered, this, &TrayController::settingsRequested);
    connect(quitAction_, &QAction::triggered, this, &TrayController::quitRequested);
}

TrayController::~TrayController() {
    if (trayIcon_ != nullptr) {
        trayIcon_->hide();
        trayIcon_->setContextMenu(nullptr);
    }
}

bool TrayController::isSystemTraySupported() {
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayController::initialize(const AppSettings& settings) {
    setHotkeyDisplay(settings.hotkey);
    trayIcon_->show();
    showInfo(
        QStringLiteral("wordSnap V1"),
        QStringLiteral("Application started and running in system tray."),
        1500);
}

void TrayController::setHotkeyDisplay(const QString& hotkey) {
    trayIcon_->setToolTip(QStringLiteral("wordSnap V1 | Hotkey: %1").arg(hotkey));

    if (captureAction_ != nullptr) {
        captureAction_->setText(QStringLiteral("Capture (%1)").arg(hotkey));
    }
}

void TrayController::showInfo(const QString& title, const QString& message, int timeoutMs) const {
    if (trayIcon_ == nullptr || !trayIcon_->isVisible()) {
        return;
    }

    trayIcon_->showMessage(title, message, QSystemTrayIcon::Information, timeoutMs);
}
