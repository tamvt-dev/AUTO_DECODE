#ifndef HISTORY_TAB_H
#define HISTORY_TAB_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "history_manager.h"

class HistoryTab : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryTab(QWidget *parent = nullptr);

    void refreshHistory();

signals:
    void itemSelected(const QString &operation, const QString &input);

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onClear();
    void onExport();
    void onRefresh();

private:
    void setupUi();
    QString formatHistoryItemText(const HistoryManager::HistoryEntry &entry) const;

    QListWidget *historyList;
    QPushButton *clearBtn;
    QPushButton *exportBtn;
    QPushButton *refreshBtn;
};

#endif // HISTORY_TAB_H
