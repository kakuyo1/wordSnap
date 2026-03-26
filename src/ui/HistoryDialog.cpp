#include "ui/HistoryDialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
QString displayTimestamp(const QDateTime& timestampUtc) {
    if (!timestampUtc.isValid()) {
        return QStringLiteral("-");
    }

    return timestampUtc.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}
} // namespace

HistoryDialog::HistoryDialog(const QVector<QueryHistoryRecord>& records, QWidget* parent)
    : QDialog(parent),
      records_(records),
      wordFilterEdit_(new QLineEdit(this)),
      timeFilterCombo_(new QComboBox(this)),
      historyTable_(new QTableWidget(this)),
      reviewButton_(new QPushButton(QStringLiteral("Review selected"), this)) {
    setWindowTitle(QStringLiteral("Query history"));
    setModal(true);
    resize(880, 540);

    auto* filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(10);

    auto* wordLabel = new QLabel(QStringLiteral("Word filter"), this);
    wordFilterEdit_->setPlaceholderText(QStringLiteral("type word or prefix"));
    wordFilterEdit_->setClearButtonEnabled(true);

    auto* timeLabel = new QLabel(QStringLiteral("Time"), this);
    timeFilterCombo_->addItem(QStringLiteral("All"), 0);
    timeFilterCombo_->addItem(QStringLiteral("Last 24 hours"), 24);
    timeFilterCombo_->addItem(QStringLiteral("Last 7 days"), 24 * 7);
    timeFilterCombo_->addItem(QStringLiteral("Last 30 days"), 24 * 30);

    filterLayout->addWidget(wordLabel);
    filterLayout->addWidget(wordFilterEdit_, 1);
    filterLayout->addWidget(timeLabel);
    filterLayout->addWidget(timeFilterCombo_);

    historyTable_->setColumnCount(4);
    historyTable_->setHorizontalHeaderLabels(QStringList{
        QStringLiteral("Time"),
        QStringLiteral("Status"),
        QStringLiteral("Word"),
        QStringLiteral("Preview")});
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    historyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->verticalHeader()->setVisible(false);
    historyTable_->horizontalHeader()->setStretchLastSection(true);
    historyTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    historyTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    historyTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    auto* clearButton = new QPushButton(QStringLiteral("Clear history"), this);
    auto* closeButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);

    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(10);
    bottomLayout->addWidget(reviewButton_);
    bottomLayout->addWidget(clearButton);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(closeButtonBox);

    connect(wordFilterEdit_, &QLineEdit::textChanged, this, &HistoryDialog::applyFilters);
    connect(timeFilterCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &HistoryDialog::applyFilters);
    connect(reviewButton_, &QPushButton::clicked, this, &HistoryDialog::onReviewClicked);
    connect(clearButton, &QPushButton::clicked, this, &HistoryDialog::onClearClicked);
    connect(closeButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(historyTable_, &QTableWidget::cellDoubleClicked, this, &HistoryDialog::onCellDoubleClicked);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);
    root->addLayout(filterLayout);
    root->addWidget(historyTable_, 1);
    root->addLayout(bottomLayout);

    applyFilters();
}

bool HistoryDialog::clearRequested() const {
    return clearRequested_;
}

bool HistoryDialog::hasReviewSelection() const {
    return hasReviewSelection_;
}

QueryHistoryRecord HistoryDialog::reviewSelection() const {
    return reviewSelection_;
}

void HistoryDialog::applyFilters() {
    filteredIndices_.clear();

    const QString keyword = wordFilterEdit_->text().trimmed();
    const int hours = timeFilterCombo_->currentData().toInt();
    const QDateTime cutoff = hours > 0
                                 ? QDateTime::currentDateTimeUtc().addSecs(-hours * 3600)
                                 : QDateTime();

    for (int index = 0; index < records_.size(); ++index) {
        const QueryHistoryRecord& record = records_.at(index);

        bool wordMatched = keyword.isEmpty();
        if (!wordMatched) {
            const QString word = displayWord(record);
            wordMatched = word.contains(keyword, Qt::CaseInsensitive)
                          || record.queryWord.contains(keyword, Qt::CaseInsensitive)
                          || record.headword.contains(keyword, Qt::CaseInsensitive);
        }

        bool timeMatched = true;
        if (hours > 0) {
            timeMatched = record.timestampUtc.isValid() && record.timestampUtc >= cutoff;
        }

        if (wordMatched && timeMatched) {
            filteredIndices_.push_back(index);
        }
    }

    reloadTable();
}

void HistoryDialog::onReviewClicked() {
    const int row = historyTable_->currentRow();
    const int index = recordIndexForRow(row);
    if (index < 0 || index >= records_.size()) {
        QMessageBox::information(
            this,
            QStringLiteral("No selection"),
            QStringLiteral("Please select one history item to review."));
        return;
    }

    reviewSelection_ = records_.at(index);
    hasReviewSelection_ = true;
    accept();
}

void HistoryDialog::onClearClicked() {
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Clear history"),
        QStringLiteral("Clear all local query history records?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    clearRequested_ = true;
    records_.clear();
    filteredIndices_.clear();
    hasReviewSelection_ = false;
    reviewSelection_ = QueryHistoryRecord{};
    reloadTable();
}

void HistoryDialog::onCellDoubleClicked(const int row, int) {
    const int index = recordIndexForRow(row);
    if (index < 0 || index >= records_.size()) {
        return;
    }

    reviewSelection_ = records_.at(index);
    hasReviewSelection_ = true;
    accept();
}

void HistoryDialog::reloadTable() {
    historyTable_->clearContents();
    historyTable_->setRowCount(filteredIndices_.size());

    for (int row = 0; row < filteredIndices_.size(); ++row) {
        const QueryHistoryRecord& record = records_.at(filteredIndices_.at(row));

        auto* timeItem = new QTableWidgetItem(displayTimestamp(record.timestampUtc));
        auto* statusItem = new QTableWidgetItem(record.statusCode);
        auto* wordItem = new QTableWidgetItem(displayWord(record));
        auto* previewItem = new QTableWidgetItem(record.preview.simplified());

        historyTable_->setItem(row, 0, timeItem);
        historyTable_->setItem(row, 1, statusItem);
        historyTable_->setItem(row, 2, wordItem);
        historyTable_->setItem(row, 3, previewItem);
    }

    reviewButton_->setEnabled(historyTable_->rowCount() > 0);
}

int HistoryDialog::recordIndexForRow(const int row) const {
    if (row < 0 || row >= filteredIndices_.size()) {
        return -1;
    }
    return filteredIndices_.at(row);
}

QString HistoryDialog::displayWord(const QueryHistoryRecord& record) {
    if (!record.headword.trimmed().isEmpty()) {
        return record.headword.trimmed();
    }
    return record.queryWord.trimmed();
}
