#pragma once

#include <QObject>
#include <QString>

#include <memory>

#include "app/AppTypes.h"

class QAction;
class QMenu;
class QSystemTrayIcon;

// Owns system tray icon/menu and exposes user-intent signals.
class TrayController final : public QObject {
    Q_OBJECT

public:
    explicit TrayController(QObject* parent = nullptr);
    ~TrayController() override;

    static bool isSystemTraySupported();

    // Applies startup state and shows tray icon.
    void initialize(const AppSettings& settings);

    // Updates tray tooltip hotkey text.
    void setHotkeyDisplay(const QString& hotkey);

    // Shows a lightweight tray balloon message.
    void showInfo(const QString& title, const QString& message, int timeoutMs = 1800) const;

signals:
    void captureRequested();
    void historyRequested();
    void settingsRequested();
    void quitRequested();

private:
    std::unique_ptr<QSystemTrayIcon> trayIcon_;
    std::unique_ptr<QMenu> trayMenu_;
    QAction* captureAction_{nullptr};
    QAction* historyAction_{nullptr};
    QAction* settingsAction_{nullptr};
    QAction* quitAction_{nullptr};
};
