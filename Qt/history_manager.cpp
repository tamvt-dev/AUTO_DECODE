#include "history_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>

HistoryManager& HistoryManager::instance()
{
    static HistoryManager manager;
    return manager;
}

HistoryManager::HistoryManager()
{
    loadFromFile();
}

void HistoryManager::addEntry(const QString &operation, const QString &format,
                               const QString &input, const QString &output,
                               double processingTimeMs, bool fromCache)
{
    HistoryEntry entry;
    entry.timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    entry.operation = operation;
    entry.format = format.simplified();
    entry.input = input;
    entry.output = output;
    entry.processingTimeMs = processingTimeMs;
    entry.fromCache = fromCache;

    m_entries.prepend(entry);  // Most recent at front
    truncateIfNeeded();
    saveToFile();
}

QList<HistoryManager::HistoryEntry> HistoryManager::getRecent(int count) const
{
    return m_entries.mid(0, qMin(count, m_entries.size()));
}

void HistoryManager::clear()
{
    m_entries.clear();
    saveToFile();
}

void HistoryManager::saveToFile()
{
    QString filePath = getHistoryFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open history file for writing:" << filePath;
        return;
    }

    QJsonArray array;
    for (const auto &entry : m_entries) {
        array.append(entryToJson(entry));
    }

    QJsonDocument doc(array);
    file.write(doc.toJson());
    file.close();
}

void HistoryManager::loadFromFile()
{
    m_entries.clear();
    QString filePath = getHistoryFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        qDebug() << "History file does not exist yet:" << filePath;
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open history file for reading:" << filePath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "History file is not valid JSON array";
        return;
    }

    QJsonArray array = doc.array();
    for (const auto &val : array) {
        if (val.isObject()) {
            m_entries.append(jsonToEntry(val.toObject()));
        }
    }
    qDebug() << "Loaded" << m_entries.size() << "history entries";
}

bool HistoryManager::exportToCsv(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open CSV file for writing:" << filePath;
        return false;
    }

    // Write header
    file.write("Timestamp,Operation,Format,Input Length,Output Length,Time (ms),From Cache\n");

    // Write entries
    for (const auto &entry : m_entries) {
        QString line = QString("%1,%2,%3,%4,%5,%6,%7\n")
            .arg(entry.timestamp)
            .arg(entry.operation)
            .arg(entry.format)
            .arg(entry.input.length())
            .arg(entry.output.length())
            .arg(entry.processingTimeMs)
            .arg(entry.fromCache ? "Yes" : "No");
        file.write(line.toUtf8());
    }

    file.close();
    return true;
}

void HistoryManager::truncateIfNeeded()
{
    if (m_entries.size() > m_maxItems) {
        m_entries.erase(m_entries.begin() + m_maxItems, m_entries.end());
    }
}

QString HistoryManager::getHistoryFilePath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return configDir + "/history.json";
}

QJsonObject HistoryManager::entryToJson(const HistoryEntry &entry) const
{
    QJsonObject obj;
    obj["timestamp"] = entry.timestamp;
    obj["operation"] = entry.operation;
    obj["format"] = entry.format;
    obj["input"] = entry.input;
    obj["output"] = entry.output;
    obj["processing_time_ms"] = entry.processingTimeMs;
    obj["from_cache"] = entry.fromCache;
    return obj;
}

HistoryManager::HistoryEntry HistoryManager::jsonToEntry(const QJsonObject &obj) const
{
    HistoryEntry entry;
    entry.timestamp = obj["timestamp"].toString();
    entry.operation = obj["operation"].toString();
    entry.format = obj["format"].toString().simplified();
    entry.input = obj["input"].toString();
    entry.output = obj["output"].toString();
    entry.processingTimeMs = obj["processing_time_ms"].toDouble();
    entry.fromCache = obj["from_cache"].toBool();
    return entry;
}
