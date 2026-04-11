#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QTabWidget>
#include <QStatusBar>
#include <QCloseEvent>
#include <QTimer>
#include <QProgressBar>
#include <QLineEdit>
#include "decoder_engine.h"
#include "history_manager.h"
#include "history_tab.h"
#include "notification_manager.h"
#include "recipe_tab.h"
#include "recipe_manager.h"
#include "batch_processor.h"

class HistoryManager;
class HistoryTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;   // <-- ADD THIS

private slots:
    void decode();
    void encode();
    void runPipeline();
    void clearDecode();
    void clearEncode();
    void copyDecode();
    void copyEncode();
    void copyPipeline();
    void refreshFormats();
    void clearCache();
    void toggleTheme(bool dark);
    void toggleConsole(bool visible);
    void openFile();
    void saveFile();
    void showAbout();

    // Batch
    void startBatch();
    void onBatchProgress(int percent);
    void onBatchStatus(const QString &message);
    void onBatchFinished(const QString &summary);

    // Hex view
    void toggleHexView(bool enabled);

    void onHistoryItemSelected(const QString &operation, const QString &input);
    void onCandidateSelected(int index);
    void onHistoryStepClicked(const QString &name, const QString &output);

private:
    void setupUi();
    QString loadStylesheetFromResource(const QString &resourcePath) const;
    void preloadThemes();
    void applyTheme(bool dark);
    void updateStatus(const QString &message, int timeout = 2000);
    void loadPreferences();
    void savePreferences();
    QString toHexView(const QString &text);

    // UI widgets
    QTextEdit *decodeInput;
    QTextEdit *decodeOutput;
    QComboBox *decodeFormatCombo;
    QLabel *detectedFormatLabel;

    QTextEdit *encodeInput;
    QTextEdit *encodeOutput;
    QComboBox *encodeFormatCombo;

    QTextEdit *pipelineInput;
    QTextEdit *pipelineOutput;
    QLabel *pipelineScoreLabel;
    QLabel *pipelineStepsLabel;
    QComboBox *candidateCombo;
    QWidget *historyFlowWidget;
    QList<DecoderEngine::CandidateResult> currentCandidates;

    QCheckBox *darkThemeCheck;
    QCheckBox *autoDecodeCheck;
    QCheckBox *notificationsCheck;
    QCheckBox *showConsoleCheck;

    QStatusBar *statusBar;

    HistoryTab *historyTab;
    RecipeTab *recipeTab;

    // Batch UI
    QTextEdit *batchRecipeJson;
    QLineEdit *batchInputDir;
    QLineEdit *batchOutputDir;
    QProgressBar *batchProgressBar;
    QTextEdit *batchLog;
    QCheckBox *hexViewCheck;

    // Engine and state
    DecoderEngine &engine;
    RecipeManager *recipeManager;
    NotificationManager &notificationManager;
    BatchProcessor *batchProcessor;
    bool autoDecode;
    bool isDarkTheme;
    bool notificationsEnabled;
    bool hexViewEnabled;
    QString darkStylesheet;
    QString lightStylesheet;
};

#endif // MAINWINDOW_H
