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
#include "decoder_engine.h"
#include "history_manager.h"
#include "history_tab.h"
#include "notification_manager.h"

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
    void onHistoryItemSelected(const QString &operation, const QString &input);

private:
    void setupUi();
    QString loadStylesheetFromResource(const QString &resourcePath) const;
    void preloadThemes();
    void applyTheme(bool dark);
    void updateStatus(const QString &message, int timeout = 2000);
    void loadPreferences();
    void savePreferences();

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

    QCheckBox *darkThemeCheck;
    QCheckBox *autoDecodeCheck;
    QCheckBox *notificationsCheck;
    QCheckBox *showConsoleCheck;

    QStatusBar *statusBar;

    HistoryTab *historyTab;

    // Engine and state
    DecoderEngine &engine;
    NotificationManager &notificationManager;
    bool autoDecode;
    bool isDarkTheme;
    bool notificationsEnabled;
    QString darkStylesheet;
    QString lightStylesheet;
};

#endif // MAINWINDOW_H
