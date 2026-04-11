#ifndef BATCH_PROCESSOR_H
#define BATCH_PROCESSOR_H

#include <QObject>
#include <QString>

class BatchProcessor : public QObject {
    Q_OBJECT

public:
    void processDir(const QString &dirPath, const QString &recipeJson, const QString &outDir);

signals:
    void progressUpdated(int percent);
    void statusUpdated(const QString &message);
    void finished(const QString &summary);
};

#endif
