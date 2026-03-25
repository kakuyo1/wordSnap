#include "ui/SettingsDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString startDirectoryFromText(const QString& text) {
    const QString trimmed = text.trimmed();
    if (!trimmed.isEmpty() && QDir(trimmed).exists()) {
        return trimmed;
    }
    return QDir::homePath();
}
} // namespace

SettingsDialog::SettingsDialog(const AppSettings& initialSettings, QWidget* parent)
    : QDialog(parent),
      hotkeyEdit_(new QLineEdit(this)),
      displayModeCombo_(new QComboBox(this)),
      starDictDirEdit_(new QLineEdit(this)),
      tessdataDirEdit_(new QLineEdit(this)),
      resultCardOpacitySlider_(new QSlider(Qt::Horizontal, this)),
      resultCardOpacityValueLabel_(new QLabel(this)) {
    setWindowTitle(QStringLiteral("wordSnap Settings"));
    setModal(true);
    resize(620, 0);

    hotkeyEdit_->setText(initialSettings.hotkey);
    hotkeyEdit_->setPlaceholderText(QStringLiteral("Shift+Alt+S"));
    hotkeyEdit_->setClearButtonEnabled(true);

    displayModeCombo_->addItem(QStringLiteral("Bilingual (EN + ZH)"), QStringLiteral("bilingual"));
    displayModeCombo_->addItem(QStringLiteral("English first"), QStringLiteral("en"));
    displayModeCombo_->addItem(QStringLiteral("Chinese first"), QStringLiteral("zh"));
    const int modeIndex = displayModeCombo_->findData(displayModeToString(initialSettings.displayMode));
    if (modeIndex >= 0) {
        displayModeCombo_->setCurrentIndex(modeIndex);
    }

    starDictDirEdit_->setText(QDir::toNativeSeparators(initialSettings.starDictDir));
    starDictDirEdit_->setClearButtonEnabled(true);
    tessdataDirEdit_->setText(QDir::toNativeSeparators(initialSettings.tessdataDir));
    tessdataDirEdit_->setClearButtonEnabled(true);

    auto* browseStarDictButton = new QPushButton(QStringLiteral("Browse..."), this);
    auto* browseTessdataButton = new QPushButton(QStringLiteral("Browse..."), this);

    auto* starDictDirRow = new QWidget(this);
    auto* starDictDirLayout = new QHBoxLayout(starDictDirRow);
    starDictDirLayout->setContentsMargins(0, 0, 0, 0);
    starDictDirLayout->addWidget(starDictDirEdit_, 1);
    starDictDirLayout->addWidget(browseStarDictButton);

    auto* tessdataDirRow = new QWidget(this);
    auto* tessdataDirLayout = new QHBoxLayout(tessdataDirRow);
    tessdataDirLayout->setContentsMargins(0, 0, 0, 0);
    tessdataDirLayout->addWidget(tessdataDirEdit_, 1);
    tessdataDirLayout->addWidget(browseTessdataButton);

    auto* formLayout = new QFormLayout();

    const int initialOpacity = clampResultCardOpacityPercent(initialSettings.resultCardOpacityPercent);
    resultCardOpacitySlider_->setRange(kMinResultCardOpacityPercent, kMaxResultCardOpacityPercent);
    resultCardOpacitySlider_->setSingleStep(1);
    resultCardOpacitySlider_->setPageStep(5);
    resultCardOpacitySlider_->setValue(initialOpacity);

    resultCardOpacityValueLabel_->setMinimumWidth(52);
    resultCardOpacityValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* opacityRow = new QWidget(this);
    auto* opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(10);
    opacityLayout->addWidget(resultCardOpacitySlider_, 1);
    opacityLayout->addWidget(resultCardOpacityValueLabel_);

    formLayout->addRow(QStringLiteral("Hotkey"), hotkeyEdit_);
    formLayout->addRow(QStringLiteral("Display mode"), displayModeCombo_);
    formLayout->addRow(QStringLiteral("StarDict folder"), starDictDirRow);
    formLayout->addRow(QStringLiteral("Tessdata folder"), tessdataDirRow);
    formLayout->addRow(QStringLiteral("Card opacity"), opacityRow);

    auto* hintLabel = new QLabel(
        QStringLiteral("Hotkey format examples: Shift+Alt+S, Ctrl+Alt+F2, Ctrl+Shift+Space"),
        this);
    hintLabel->setWordWrap(true);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(browseStarDictButton, &QPushButton::clicked, this, &SettingsDialog::browseStarDictDirectory);
    connect(browseTessdataButton, &QPushButton::clicked, this, &SettingsDialog::browseTessdataDirectory);
    connect(resultCardOpacitySlider_, &QSlider::valueChanged, this, &SettingsDialog::onResultCardOpacityChanged);

    onResultCardOpacityChanged(initialOpacity);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->addLayout(formLayout);
    rootLayout->addWidget(hintLabel);
    rootLayout->addWidget(buttonBox);
}

AppSettings SettingsDialog::editedSettings() const {
    AppSettings edited;
    edited.hotkey = hotkeyEdit_->text().trimmed();
    edited.displayMode = displayModeFromString(displayModeCombo_->currentData().toString());
    edited.starDictDir = QDir::fromNativeSeparators(starDictDirEdit_->text().trimmed());
    edited.tessdataDir = QDir::fromNativeSeparators(tessdataDirEdit_->text().trimmed());
    edited.resultCardOpacityPercent = clampResultCardOpacityPercent(resultCardOpacitySlider_->value());
    return edited;
}

void SettingsDialog::accept() {
    if (hotkeyEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Invalid hotkey"),
            QStringLiteral("Hotkey cannot be empty."));
        hotkeyEdit_->setFocus();
        return;
    }

    QDialog::accept();
}

void SettingsDialog::browseStarDictDirectory() {
    const QString selectedDirectory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select StarDict directory"),
        startDirectoryFromText(starDictDirEdit_->text()),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!selectedDirectory.isEmpty()) {
        starDictDirEdit_->setText(QDir::toNativeSeparators(selectedDirectory));
    }
}

void SettingsDialog::browseTessdataDirectory() {
    const QString selectedDirectory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select tessdata directory"),
        startDirectoryFromText(tessdataDirEdit_->text()),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!selectedDirectory.isEmpty()) {
        tessdataDirEdit_->setText(QDir::toNativeSeparators(selectedDirectory));
    }
}

void SettingsDialog::onResultCardOpacityChanged(const int value) {
    const int clamped = clampResultCardOpacityPercent(value);
    resultCardOpacityValueLabel_->setText(QStringLiteral("%1%").arg(clamped));
}
