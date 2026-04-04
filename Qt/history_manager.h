#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <QString>
#include <QList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class HistoryManager
{
public:
    static HistoryManager& instance();

    struct HistoryEntry {
        QString timestamp;
        QString operation;  // "decode", "encode", "pipeline"
        QString format;
        QString input;
        QString output;
        double processingTimeMs = 0.0;
        bool fromCache = false;
    };

    void addEntry(const QString &operation, const QString &format,
                  const QString &input, const QString &output,
                  double processingTimeMs, bool fromCache);

    QList<HistoryEntry> getRecent(int count = 15) const;
    QList<HistoryEntry> getAll() const { return m_entries; }

    void clear();
    void saveToFile();
    void loadFromFile();

    // Export to CSV
    bool exportToCsv(const QString &filePath) const;

    int maxItems() const { return m_maxItems; }
    void setMaxItems(int max) { m_maxItems = max; }

private:
    HistoryManager();
    void truncateIfNeeded();
    QString getHistoryFilePath() const;
    QJsonObject entryToJson(const HistoryEntry &entry) const;
    HistoryEntry jsonToEntry(const QJsonObject &obj) const;

    QList<HistoryEntry> m_entries;
    int m_maxItems = 500;
};

#endif // HISTORY_MANAGER_H
