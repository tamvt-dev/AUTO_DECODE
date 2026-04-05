#include "history_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

HistoryTab::HistoryTab(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    refreshHistory();
}

void HistoryTab::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *titleLabel = new QLabel("Recent Operations");
    titleLabel->setProperty("class", "sectionLabel");
    mainLayout->addWidget(titleLabel);

    historyList = new QListWidget;
    historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(historyList);

    connect(historyList, &QListWidget::itemDoubleClicked, this, &HistoryTab::onItemDoubleClicked);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    refreshBtn = new QPushButton("Refresh");
    clearBtn = new QPushButton("Clear All");
    exportBtn = new QPushButton("Export as CSV");

    buttonLayout->addWidget(refreshBtn);
    buttonLayout->addWidget(clearBtn);
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    connect(refreshBtn, &QPushButton::clicked, this, &HistoryTab::onRefresh);
    connect(clearBtn, &QPushButton::clicked, this, &HistoryTab::onClear);
    connect(exportBtn, &QPushButton::clicked, this, &HistoryTab::onExport);
}

void HistoryTab::refreshHistory()
{
    historyList->clear();

    auto &manager = HistoryManager::instance();
    auto entries = manager.getAll();

    if (entries.isEmpty()) {
        historyList->addItem("No history yet. Perform encode/decode operations to see them here.");
        return;
    }

    for (const auto &entry : entries) {
        QListWidgetItem *item = new QListWidgetItem(formatHistoryItemText(entry));
        item->setData(Qt::UserRole, entry.operation);
        item->setData(Qt::UserRole + 1, entry.input);
        item->setData(Qt::UserRole + 2, entry.format);
        historyList->addItem(item);
    }
}

QString HistoryTab::formatHistoryItemText(const HistoryManager::HistoryEntry &entry) const
{
    const QString normalizedInput = entry.input.simplified();
    const QString normalizedOutput = entry.output.simplified();
    const QString normalizedFormat = entry.format.simplified();
    const QString inputPreview = normalizedInput.length() > 30 ? normalizedInput.left(30) + "..." : normalizedInput;
    const QString outputPreview = normalizedOutput.length() > 30 ? normalizedOutput.left(30) + "..." : normalizedOutput;

    return QString("[%1] %2 -> %3 | %4 | %5 ms")
        .arg(entry.operation.toUpper())
        .arg(inputPreview)
        .arg(outputPreview)
        .arg(normalizedFormat)
        .arg(entry.processingTimeMs, 0, 'f', 2);
}

void HistoryTab::onItemDoubleClicked(QListWidgetItem *item)
{
    const QString operation = item->data(Qt::UserRole).toString();
    const QString input = item->data(Qt::UserRole + 1).toString();
    emit itemSelected(operation, input);
}

void HistoryTab::onClear()
{
    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear History",
        "Are you sure you want to clear all history?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        HistoryManager::instance().clear();
        refreshHistory();
    }
}

void HistoryTab::onExport()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export History as CSV",
        QString(),
        "CSV Files (*.csv);;All Files (*)"
    );

    if (!filePath.isEmpty()) {
        if (HistoryManager::instance().exportToCsv(filePath)) {
            QMessageBox::information(this, "Export Successful",
                QString("History exported to:\n%1").arg(filePath));
        } else {
            QMessageBox::warning(this, "Export Failed",
                "Failed to export history to CSV file.");
        }
    }
}

void HistoryTab::onRefresh()
{
    refreshHistory();
}
