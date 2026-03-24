#include "platform/win/GlobalHotkeyManager.h"

#include <QStringList>

#include <utility>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cwchar>
#endif

namespace {
#ifdef Q_OS_WIN
constexpr int kGlobalHotkeyId = 0xA11F;
constexpr wchar_t kHotkeyWindowClassName[] = L"WordSnapV1HotkeyMessageWindow";

LRESULT CALLBACK HotkeyWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    auto* instance = reinterpret_cast<GlobalHotkeyManager*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (instance != nullptr && msg == WM_HOTKEY) {
        GlobalHotkeyManager::handleWinHotkeyMessage(instance, static_cast<unsigned long>(wParam));
        return 0;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool ensureHotkeyWindowClassRegistered() {
    static bool registered = false;
    static bool attempted = false;
    if (attempted) {
        return registered;
    }

    attempted = true;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = HotkeyWindowProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = kHotkeyWindowClassName;

    const ATOM atom = ::RegisterClassExW(&wc);
    if (atom != 0 || ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        registered = true;
    }

    return registered;
}
#endif
}

GlobalHotkeyManager::GlobalHotkeyManager(QObject* parent)
    : QObject(parent) {}

GlobalHotkeyManager::~GlobalHotkeyManager() {
    unregisterHotkey();
    destroyMessageWindow();
}

bool GlobalHotkeyManager::registerHotkey(const QString& hotkeyText, QString* errorMessage) {
    unregisterHotkey();

#ifndef Q_OS_WIN
    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Global hotkey is only supported on Windows in V1.");
    }
    return false;
#else
    if (!ensureMessageWindow(errorMessage)) {
        return false;
    }

    const std::optional<ParsedHotkey> parsed = parseHotkey(hotkeyText);
    if (!parsed.has_value()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Invalid hotkey format: %1").arg(hotkeyText);
        }
        return false;
    }

    const HWND hwnd = reinterpret_cast<HWND>(messageWindowHandle_);
    const UINT modifiersNoRepeat = parsed->modifiers | MOD_NOREPEAT;
    if (::RegisterHotKey(hwnd, kGlobalHotkeyId, modifiersNoRepeat, parsed->virtualKey) == FALSE) {
        const DWORD firstError = ::GetLastError();

        // Compatibility fallback for environments that do not accept MOD_NOREPEAT.
        if (::RegisterHotKey(hwnd, kGlobalHotkeyId, parsed->modifiers, parsed->virtualKey) == FALSE) {
            const DWORD secondError = ::GetLastError();
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("RegisterHotKey failed, win32 error=%1 (fallback error=%2)")
                                    .arg(secondError)
                                    .arg(firstError);
            }
            return false;
        }
    }

    registered_ = true;
    hotkeyText_ = hotkeyText.trimmed();
    return true;
#endif
}

void GlobalHotkeyManager::unregisterHotkey() {
#ifdef Q_OS_WIN
    if (registered_ && messageWindowHandle_ != nullptr) {
        const HWND hwnd = reinterpret_cast<HWND>(messageWindowHandle_);
        ::UnregisterHotKey(hwnd, kGlobalHotkeyId);
    }
#endif

    registered_ = false;
    hotkeyText_.clear();
}

bool GlobalHotkeyManager::isRegistered() const noexcept {
    return registered_;
}

QString GlobalHotkeyManager::hotkeyText() const {
    return hotkeyText_;
}

bool GlobalHotkeyManager::ensureMessageWindow(QString* errorMessage) {
#ifndef Q_OS_WIN
    Q_UNUSED(errorMessage);
    return false;
#else
    if (messageWindowHandle_ != nullptr) {
        return true;
    }

    if (!ensureHotkeyWindowClassRegistered()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to register hotkey window class, win32 error=%1")
                                .arg(::GetLastError());
        }
        return false;
    }

    const HWND hwnd = ::CreateWindowExW(
        0,
        kHotkeyWindowClassName,
        L"",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        ::GetModuleHandleW(nullptr),
        this);

    if (hwnd == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to create hotkey message window, win32 error=%1")
                                .arg(::GetLastError());
        }
        return false;
    }

    messageWindowHandle_ = hwnd;
    return true;
#endif
}

void GlobalHotkeyManager::destroyMessageWindow() {
#ifdef Q_OS_WIN
    if (messageWindowHandle_ != nullptr) {
        const HWND hwnd = reinterpret_cast<HWND>(messageWindowHandle_);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        ::DestroyWindow(hwnd);
        messageWindowHandle_ = nullptr;
    }
#endif
}

void GlobalHotkeyManager::handleWinHotkeyMessage(void* instancePtr, unsigned long hotkeyId) {
#ifdef Q_OS_WIN
    if (instancePtr == nullptr) {
        return;
    }

    if (hotkeyId == static_cast<unsigned long>(kGlobalHotkeyId)) {
        auto* instance = static_cast<GlobalHotkeyManager*>(instancePtr);
        emit instance->hotkeyPressed();
    }
#else
    Q_UNUSED(instancePtr);
    Q_UNUSED(hotkeyId);
#endif
}

std::optional<GlobalHotkeyManager::ParsedHotkey> GlobalHotkeyManager::parseHotkey(const QString& hotkeyText) {
#ifdef Q_OS_WIN
    const QStringList tokens = hotkeyText.split('+', Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return std::nullopt;
    }

    unsigned int modifiers = 0;
    std::optional<unsigned int> virtualKey;

    for (QString token : tokens) {
        token = token.trimmed().toUpper();
        if (token.isEmpty()) {
            continue;
        }

        if (token == QStringLiteral("SHIFT")) {
            modifiers |= MOD_SHIFT;
            continue;
        }
        if (token == QStringLiteral("ALT")) {
            modifiers |= MOD_ALT;
            continue;
        }
        if (token == QStringLiteral("CTRL") || token == QStringLiteral("CONTROL")) {
            modifiers |= MOD_CONTROL;
            continue;
        }
        if (token == QStringLiteral("WIN") || token == QStringLiteral("WINDOWS") || token == QStringLiteral("META")) {
            modifiers |= MOD_WIN;
            continue;
        }

        const std::optional<unsigned int> maybeVk = parseMainKey(token);
        if (!maybeVk.has_value()) {
            return std::nullopt;
        }
        if (virtualKey.has_value()) {
            return std::nullopt;
        }
        virtualKey = maybeVk;
    }

    if (!virtualKey.has_value() || modifiers == 0) {
        return std::nullopt;
    }

    return ParsedHotkey{modifiers, virtualKey.value()};
#else
    Q_UNUSED(hotkeyText);
    return std::nullopt;
#endif
}

std::optional<unsigned int> GlobalHotkeyManager::parseMainKey(const QString& keyToken) {
#ifndef Q_OS_WIN
    Q_UNUSED(keyToken);
    return std::nullopt;
#else
    if (keyToken.size() == 1) {
        const QChar ch = keyToken.at(0).toUpper();
        if (ch.isLetter() || ch.isDigit()) {
            return static_cast<unsigned int>(ch.unicode());
        }
    }

    if (keyToken.startsWith('F')) {
        bool ok = false;
        const int fn = keyToken.mid(1).toInt(&ok);
        if (ok && fn >= 1 && fn <= 24) {
            return static_cast<unsigned int>(VK_F1 + (fn - 1));
        }
    }

    if (keyToken == QStringLiteral("SPACE")) {
        return static_cast<unsigned int>(VK_SPACE);
    }
    if (keyToken == QStringLiteral("TAB")) {
        return static_cast<unsigned int>(VK_TAB);
    }
    if (keyToken == QStringLiteral("ENTER") || keyToken == QStringLiteral("RETURN")) {
        return static_cast<unsigned int>(VK_RETURN);
    }
    if (keyToken == QStringLiteral("ESC") || keyToken == QStringLiteral("ESCAPE")) {
        return static_cast<unsigned int>(VK_ESCAPE);
    }

    return std::nullopt;
#endif
}
