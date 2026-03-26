#pragma once

#include <QDialog>
#include <QVector>

#include "app/AppTypes.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Query history browser with filtering and quick replay.
class HistoryDialog final : public QDialog {
    Q_OBJECT

public:
    explicit HistoryDialog(const QVector<QueryHistoryRecord>& records, QWidget* parent = nullptr);

    bool clearRequested() const;
    bool hasReviewSelection() const;
    QueryHistoryRecord reviewSelection() const;

private slots:
    void applyFilters();
    void onReviewClicked();
    void onClearClicked();
    void onCellDoubleClicked(int row, int column);

private:
    void reloadTable();
    int recordIndexForRow(int row) const;
    static QString displayWord(const QueryHistoryRecord& record);

    QVector<QueryHistoryRecord> records_;
    QVector<int> filteredIndices_;

    bool clearRequested_{false};
    bool hasReviewSelection_{false};
    QueryHistoryRecord reviewSelection_;

    QLineEdit* wordFilterEdit_{nullptr};
    QComboBox* timeFilterCombo_{nullptr};
    QTableWidget* historyTable_{nullptr};
    QPushButton* reviewButton_{nullptr};
};
