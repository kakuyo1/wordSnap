#pragma once

#include <QObject>
#include <QString>

#include <optional>

// Windows global hotkey wrapper based on RegisterHotKey/WM_HOTKEY.
// This class only provides platform events and does not contain business logic.
class GlobalHotkeyManager final : public QObject {
    Q_OBJECT

public:
    explicit GlobalHotkeyManager(QObject* parent = nullptr);
    ~GlobalHotkeyManager() override;

    // Registers a system-wide hotkey from a string like "Shift+Alt+S".
    bool registerHotkey(const QString& hotkeyText, QString* errorMessage = nullptr);

    // Unregisters the current hotkey if present.
    void unregisterHotkey();

    bool isRegistered() const noexcept;
    QString hotkeyText() const;

    // Called by Win32 message window procedure when WM_HOTKEY arrives.
    static void handleWinHotkeyMessage(void* instancePtr, unsigned long hotkeyId);

signals:
    // Emitted when the registered global hotkey is pressed.
    void hotkeyPressed();

private:
    struct ParsedHotkey {
        unsigned int modifiers{0};
        unsigned int virtualKey{0};
    };

    static std::optional<ParsedHotkey> parseHotkey(const QString& hotkeyText);
    static std::optional<unsigned int> parseMainKey(const QString& keyToken);

    bool ensureMessageWindow(QString* errorMessage);
    void destroyMessageWindow();

private:
    bool registered_{false};
    QString hotkeyText_;

#ifdef Q_OS_WIN
    void* messageWindowHandle_{nullptr};
#endif
};
