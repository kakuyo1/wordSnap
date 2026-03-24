#pragma once

#include <QObject>
#include <QRect>
#include <QString>

#include <memory>

#include "app/AppTypes.h"

class GlobalHotkeyManager;
class CaptureOverlay;
class ScreenCaptureService;
class ImagePreprocessor;
class OcrService;
class WordNormalizer;
class DictionaryService;
class TrayController;
class ResultCardWidget;
class SettingsService;

// Coordinates startup and top-level flow between infrastructure modules.
class AppController final : public QObject {
    Q_OBJECT

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    // Initializes tray and global hotkey.
    // Returns false only for fatal startup failures.
    bool initialize(QString* warningMessage = nullptr);

private slots:
    void onHotkeyPressed();
    void onRegionSelected(const QRect& globalRect);
    void onCaptureCanceled();
    void onSettingsRequested();

private:
    AppSettings settings_;
    std::unique_ptr<SettingsService> settingsService_;
    std::unique_ptr<TrayController> trayController_;
    std::unique_ptr<GlobalHotkeyManager> hotkeyManager_;
    std::unique_ptr<CaptureOverlay> captureOverlay_;
    std::unique_ptr<ScreenCaptureService> screenCaptureService_;
    std::unique_ptr<ImagePreprocessor> imagePreprocessor_;
    std::unique_ptr<OcrService> ocrService_;
    std::unique_ptr<WordNormalizer> wordNormalizer_;
    std::unique_ptr<DictionaryService> dictionaryService_;
    std::unique_ptr<ResultCardWidget> resultCardWidget_;
};
