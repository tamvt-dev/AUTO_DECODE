#include "batch_processor.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent>
#include <QFuture>
#include <QThreadPool>
#include <QMutex>
#include <QAtomicInt>
#include "decoder_engine.h"
#include "console_manager.h"

void BatchProcessor::processDir(const QString &dirPath, const QString &recipeJson, const QString &outDir) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        emit statusUpdated("Input directory does not exist");
        return;
    }

    QDir out(outDir);
    if (!out.exists() && !out.mkpath(".")) {
        emit statusUpdated("Cannot create output directory: " + outDir);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(recipeJson.toUtf8());
    if (!doc.isObject()) {
        emit statusUpdated("Invalid recipe JSON");
        return;
    }
    QJsonObject recipe = doc.object();

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.*", QDir::Files);
    int total = files.size();
    QAtomicInt processed(0);
    QMutex progressMutex;

    emit statusUpdated(QString("Processing %1 files in parallel...").arg(total));

    // Parallel batch processing using QtConcurrent
    QList<QFuture<void>> futures;
    
    for (const QFileInfo &fileInfo : files) {
        QFuture<void> future = QtConcurrent::run([=, &processed, &progressMutex, this]() {
            QString inputFile = fileInfo.absoluteFilePath();
            QString outputFile = out.absoluteFilePath(fileInfo.fileName());

            // Read input
            QFile input(inputFile);
            if (!input.open(QIODevice::ReadOnly)) {
                processed.fetchAndAddRelaxed(1);
                return;
            }
            QString content = QString::fromUtf8(input.readAll());
            input.close();

            // Apply recipe via engine
            DecoderEngine &engine = DecoderEngine::instance();
            QJsonDocument recipeDoc(recipe);
            QString recipeJsonStr = QString::fromUtf8(recipeDoc.toJson(QJsonDocument::Compact));
            auto result = engine.executeRecipe(recipeJsonStr, content);

            // Write output
            QFile output(outputFile);
            if (result.success && output.open(QIODevice::WriteOnly)) {
                output.write(result.output.toUtf8());
                output.close();
            }

            int current = processed.fetchAndAddRelaxed(1) + 1;
            int percent = (current * 100) / total;
            
            QMutexLocker locker(&progressMutex);
            emit progressUpdated(percent);
            emit statusUpdated(QString("Processed %1/%2: %3").arg(current).arg(total).arg(fileInfo.fileName()));
        });
        
        futures.append(future);
    }

    // Wait for all tasks to complete
    for (auto &future : futures) {
        future.waitForFinished();
    }

    emit finished(QString("%1 files processed to %2").arg(processed.loadRelaxed()).arg(outDir));
}

