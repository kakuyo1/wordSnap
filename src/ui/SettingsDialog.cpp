#include "ui/SettingsDialog.h"

#include <QComboBox>
#include <QCheckBox>
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
#include <QSpinBox>
#include <QUrl>
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
      starDictDirEdit_(new QLineEdit(this)),
      tessdataDirEdit_(new QLineEdit(this)),
      resultCardStyleCombo_(new QComboBox(this)),
      resultCardOpacitySlider_(new QSlider(Qt::Horizontal, this)),
      resultCardOpacityValueLabel_(new QLabel(this)),
      resultCardDurationSpinBox_(new QSpinBox(this)),
      queryHistoryLimitSpinBox_(new QSpinBox(this)),
      aiAssistEnabledCheckBox_(new QCheckBox(this)),
      aiApiKeyEdit_(new QLineEdit(this)),
      aiBaseUrlEdit_(new QLineEdit(this)),
      aiModelEdit_(new QLineEdit(this)),
      aiTimeoutSpinBox_(new QSpinBox(this)) {
    setWindowTitle(QStringLiteral("wordSnap Settings"));
    setModal(true);
    resize(620, 0);

    hotkeyEdit_->setText(initialSettings.hotkey);
    hotkeyEdit_->setPlaceholderText(QStringLiteral("Shift+Alt+S"));
    hotkeyEdit_->setClearButtonEnabled(true);

    resultCardStyleCombo_->addItem(QStringLiteral("Kraft paper"), QStringLiteral("kraft_paper"));
    resultCardStyleCombo_->addItem(QStringLiteral("Glassmorphism"), QStringLiteral("glassmorphism"));
    resultCardStyleCombo_->addItem(QStringLiteral("Terminal"), QStringLiteral("terminal"));
    const int styleIndex = resultCardStyleCombo_->findData(resultCardStyleToString(initialSettings.resultCardStyle));
    if (styleIndex >= 0) {
        resultCardStyleCombo_->setCurrentIndex(styleIndex);
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

    queryHistoryLimitSpinBox_->setRange(kMinQueryHistoryLimit, kMaxQueryHistoryLimit);
    queryHistoryLimitSpinBox_->setSingleStep(25);
    queryHistoryLimitSpinBox_->setValue(clampQueryHistoryLimit(initialSettings.queryHistoryLimit));

    resultCardDurationSpinBox_->setRange(kMinResultCardDurationMs, kMaxResultCardDurationMs);
    resultCardDurationSpinBox_->setSingleStep(500);
    resultCardDurationSpinBox_->setSuffix(QStringLiteral(" ms"));
    resultCardDurationSpinBox_->setValue(clampResultCardDurationMs(initialSettings.resultCardDurationMs));

    aiAssistEnabledCheckBox_->setChecked(initialSettings.aiAssistEnabled);

    aiApiKeyEdit_->setText(initialSettings.aiApiKey);
    aiApiKeyEdit_->setClearButtonEnabled(true);
    aiApiKeyEdit_->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    aiBaseUrlEdit_->setText(initialSettings.aiBaseUrl);
    aiBaseUrlEdit_->setClearButtonEnabled(true);

    aiModelEdit_->setText(initialSettings.aiModel);
    aiModelEdit_->setClearButtonEnabled(true);

    aiTimeoutSpinBox_->setRange(kMinAiTimeoutMs, kMaxAiTimeoutMs);
    aiTimeoutSpinBox_->setSingleStep(500);
    aiTimeoutSpinBox_->setSuffix(QStringLiteral(" ms"));
    aiTimeoutSpinBox_->setValue(clampAiTimeoutMs(initialSettings.aiTimeoutMs));

    auto* opacityRow = new QWidget(this);
    auto* opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(10);
    opacityLayout->addWidget(resultCardOpacitySlider_, 1);
    opacityLayout->addWidget(resultCardOpacityValueLabel_);

    formLayout->addRow(QStringLiteral("Hotkey"), hotkeyEdit_);
    formLayout->addRow(QStringLiteral("Result card style"), resultCardStyleCombo_);
    formLayout->addRow(QStringLiteral("StarDict folder"), starDictDirRow);
    formLayout->addRow(QStringLiteral("Tessdata folder"), tessdataDirRow);
    formLayout->addRow(QStringLiteral("Card opacity"), opacityRow);
    formLayout->addRow(QStringLiteral("Card duration"), resultCardDurationSpinBox_);
    formLayout->addRow(QStringLiteral("History size (N)"), queryHistoryLimitSpinBox_);
    formLayout->addRow(QStringLiteral("Enable AI assist"), aiAssistEnabledCheckBox_);
    formLayout->addRow(QStringLiteral("AI API key"), aiApiKeyEdit_);
    formLayout->addRow(QStringLiteral("AI endpoint"), aiBaseUrlEdit_);
    formLayout->addRow(QStringLiteral("AI model"), aiModelEdit_);
    formLayout->addRow(QStringLiteral("AI timeout"), aiTimeoutSpinBox_);

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
    connect(aiAssistEnabledCheckBox_, &QCheckBox::toggled, this, &SettingsDialog::onAiAssistEnabledChanged);

    onResultCardOpacityChanged(initialOpacity);
    onAiAssistEnabledChanged(initialSettings.aiAssistEnabled);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->addLayout(formLayout);
    rootLayout->addWidget(hintLabel);
    rootLayout->addWidget(buttonBox);
}

AppSettings SettingsDialog::editedSettings() const {
    AppSettings edited;
    edited.hotkey = hotkeyEdit_->text().trimmed();
    edited.starDictDir = QDir::fromNativeSeparators(starDictDirEdit_->text().trimmed());
    edited.tessdataDir = QDir::fromNativeSeparators(tessdataDirEdit_->text().trimmed());
    edited.resultCardOpacityPercent = clampResultCardOpacityPercent(resultCardOpacitySlider_->value());
    edited.resultCardDurationMs = clampResultCardDurationMs(resultCardDurationSpinBox_->value());
    edited.resultCardStyle = resultCardStyleFromString(resultCardStyleCombo_->currentData().toString());
    edited.queryHistoryLimit = clampQueryHistoryLimit(queryHistoryLimitSpinBox_->value());
    edited.aiAssistEnabled = aiAssistEnabledCheckBox_->isChecked();
    edited.aiApiKey = aiApiKeyEdit_->text().trimmed();
    edited.aiBaseUrl = aiBaseUrlEdit_->text().trimmed();
    edited.aiModel = aiModelEdit_->text().trimmed();
    edited.aiTimeoutMs = clampAiTimeoutMs(aiTimeoutSpinBox_->value());
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

    if (aiAssistEnabledCheckBox_->isChecked()) {
        if (aiApiKeyEdit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(
                this,
                QStringLiteral("Invalid AI API key"),
                QStringLiteral("AI API key cannot be empty when AI assist is enabled."));
            aiApiKeyEdit_->setFocus();
            return;
        }

        const QString endpoint = aiBaseUrlEdit_->text().trimmed();
        const QUrl endpointUrl(endpoint);
        const QString scheme = endpointUrl.scheme().toLower();
        if (endpoint.isEmpty() || !endpointUrl.isValid()
            || (scheme != QStringLiteral("https") && scheme != QStringLiteral("http"))) {
            QMessageBox::warning(
                this,
                QStringLiteral("Invalid AI endpoint"),
                QStringLiteral("AI endpoint must be a valid HTTP or HTTPS URL."));
            aiBaseUrlEdit_->setFocus();
            return;
        }

        if (aiModelEdit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(
                this,
                QStringLiteral("Invalid AI model"),
                QStringLiteral("AI model cannot be empty when AI assist is enabled."));
            aiModelEdit_->setFocus();
            return;
        }
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

void SettingsDialog::onAiAssistEnabledChanged(const bool checked) {
    aiApiKeyEdit_->setEnabled(checked);
    aiBaseUrlEdit_->setEnabled(checked);
    aiModelEdit_->setEnabled(checked);
    aiTimeoutSpinBox_->setEnabled(checked);
}
