#pragma once

#include <QDialog>

#include "app/AppTypes.h"

class QComboBox;
class QLineEdit;

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(const AppSettings& initialSettings, QWidget* parent = nullptr);

    AppSettings editedSettings() const;

protected:
    void accept() override;

private slots:
    void browseStarDictDirectory();
    void browseTessdataDirectory();

private:
    QLineEdit* hotkeyEdit_{nullptr};
    QComboBox* displayModeCombo_{nullptr};
    QLineEdit* starDictDirEdit_{nullptr};
    QLineEdit* tessdataDirEdit_{nullptr};
};
