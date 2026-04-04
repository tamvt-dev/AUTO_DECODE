#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QTextStream>
#include <QFile>
#include <QCloseEvent>
#include <QFrame>
#include <QFontDatabase>
#include <QSettings>
#include "history_manager.h"
#include "history_tab.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , engine(DecoderEngine::instance())
    , notificationManager(NotificationManager::instance())
    , autoDecode(false)
    , isDarkTheme(true)
    , notificationsEnabled(true)
{
    setupUi();

    // Initialize notification manager
    notificationManager.setParentWidget(this);

    // Load preferences before applying the initial theme.
    loadPreferences();
    applyTheme(isDarkTheme);

    // Initialize format lists
    engine.refreshFormats();

    // Populate format combos
    decodeFormatCombo->addItems(engine.decodeFormats());
    encodeFormatCombo->addItems(engine.encodeFormats());

    // Connect signals
    connect(decodeInput, &QTextEdit::textChanged, this, [this]() {
        if (autoDecode) decode();
    });
    connect(encodeInput, &QTextEdit::textChanged, this, [this]() {
        if (autoDecode) encode();
    });
    connect(decodeFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::decode);
    connect(encodeFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::encode);
    connect(autoDecodeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        autoDecode = checked;
        updateStatus(checked ? "Auto-decode enabled" : "Auto-decode disabled");
    });
    connect(darkThemeCheck, &QCheckBox::toggled, this, &MainWindow::toggleTheme);
    connect(notificationsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        notificationsEnabled = checked;
        savePreferences();
        updateStatus(checked ? "Notifications enabled" : "Notifications disabled");
    });
    connect(historyTab, &HistoryTab::itemSelected, this, &MainWindow::onHistoryItemSelected);
}

MainWindow::~MainWindow()
{
    // No cleanup here – core is cleaned in main()
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    savePreferences();
    HistoryManager::instance().saveToFile();
    event->accept();
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    central->setProperty("class", "appShell");
    central->setContentsMargins(16, 16, 16, 16);  // md spacing (16px base)
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(12);  // compact density

    QTabWidget *tabs = new QTabWidget(this);
    tabs->setProperty("class", "workspaceTabs");
    tabs->setDocumentMode(true);
    tabs->setTabPosition(QTabWidget::North);

    QLabel *titleLabel = new QLabel("AUTO DECODER");
    titleLabel->setProperty("class", "appTitle");
    QLabel *subtitleLabel = new QLabel("Decode, encode, and inspect transformations in a compact mono workspace.");
    subtitleLabel->setProperty("class", "appSubtitle");
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);

    // ---------- Decode Tab ----------
    QWidget *decodeTab = new QWidget;
    QVBoxLayout *decodeLayout = new QVBoxLayout(decodeTab);
    decodeLayout->setSpacing(8);  // compact density

    QLabel *inputLabel = new QLabel("Encoded Text:");
    inputLabel->setProperty("class", "sectionLabel");
    decodeLayout->addWidget(inputLabel);

    decodeInput = new QTextEdit;
    decodeInput->setProperty("class", "editor");
    decodeInput->setPlaceholderText("Enter Base64, Hex, Binary, Morse, or any encoded text...");
    decodeInput->setMinimumHeight(120);
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    decodeInput->setFont(fixedFont);
    decodeLayout->addWidget(decodeInput);

    QHBoxLayout *formatLayout = new QHBoxLayout;
    QLabel *formatLabel = new QLabel("Format:");
    decodeFormatCombo = new QComboBox;
    decodeFormatCombo->setMinimumWidth(150);
    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(decodeFormatCombo);
    formatLayout->addStretch();
    decodeLayout->addLayout(formatLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *decodeBtn = new QPushButton("Decode");
    decodeBtn->setProperty("class", "primary");
    QPushButton *copyDecodeBtn = new QPushButton("Copy");
    copyDecodeBtn->setProperty("class", "secondary");
    QPushButton *clearDecodeBtn = new QPushButton("Clear");
    clearDecodeBtn->setProperty("class", "secondary");
    QPushButton *openFileBtn = new QPushButton("Open File");
    openFileBtn->setProperty("class", "secondary");
    buttonLayout->addWidget(decodeBtn);
    buttonLayout->addWidget(copyDecodeBtn);
    buttonLayout->addWidget(clearDecodeBtn);
    buttonLayout->addWidget(openFileBtn);
    buttonLayout->addStretch();
    decodeLayout->addLayout(buttonLayout);

    detectedFormatLabel = new QLabel("Detected format: —");
    detectedFormatLabel->setProperty("class", "infoLabel");
    decodeLayout->addWidget(detectedFormatLabel);

    QLabel *outputLabel = new QLabel("Decoded Result:");
    outputLabel->setProperty("class", "sectionLabel");
    decodeLayout->addWidget(outputLabel);

    decodeOutput = new QTextEdit;
    decodeOutput->setProperty("class", "editor output");
    decodeOutput->setReadOnly(true);
    decodeOutput->setFont(fixedFont);
    decodeOutput->setMinimumHeight(150);
    decodeLayout->addWidget(decodeOutput);

    connect(decodeBtn, &QPushButton::clicked, this, &MainWindow::decode);
    connect(copyDecodeBtn, &QPushButton::clicked, this, &MainWindow::copyDecode);
    connect(clearDecodeBtn, &QPushButton::clicked, this, &MainWindow::clearDecode);
    connect(openFileBtn, &QPushButton::clicked, this, &MainWindow::openFile);

    tabs->addTab(decodeTab, "Decode");

    // ---------- Encode Tab ----------
    QWidget *encodeTab = new QWidget;
    QVBoxLayout *encodeLayout = new QVBoxLayout(encodeTab);
    encodeLayout->setSpacing(8);  // compact density

    QLabel *encodeInputLabel = new QLabel("Plain Text:");
    encodeInputLabel->setProperty("class", "sectionLabel");
    encodeLayout->addWidget(encodeInputLabel);

    encodeInput = new QTextEdit;
    encodeInput->setProperty("class", "editor");
    encodeInput->setPlaceholderText("Enter text to encode...");
    encodeInput->setMinimumHeight(120);
    encodeInput->setFont(fixedFont);
    encodeLayout->addWidget(encodeInput);

    QHBoxLayout *encodeFormatLayout = new QHBoxLayout;
    QLabel *encodeFormatLabel = new QLabel("Format:");
    encodeFormatCombo = new QComboBox;
    encodeFormatCombo->setMinimumWidth(150);
    encodeFormatLayout->addWidget(encodeFormatLabel);
    encodeFormatLayout->addWidget(encodeFormatCombo);
    encodeFormatLayout->addStretch();
    encodeLayout->addLayout(encodeFormatLayout);

    QHBoxLayout *encodeButtonLayout = new QHBoxLayout;
    QPushButton *encodeBtn = new QPushButton("Encode");
    encodeBtn->setProperty("class", "primary");
    QPushButton *copyEncodeBtn = new QPushButton("Copy");
    copyEncodeBtn->setProperty("class", "secondary");
    QPushButton *clearEncodeBtn = new QPushButton("Clear");
    clearEncodeBtn->setProperty("class", "secondary");
    QPushButton *saveFileBtn = new QPushButton("Save to File");
    saveFileBtn->setProperty("class", "secondary");
    encodeButtonLayout->addWidget(encodeBtn);
    encodeButtonLayout->addWidget(copyEncodeBtn);
    encodeButtonLayout->addWidget(clearEncodeBtn);
    encodeButtonLayout->addWidget(saveFileBtn);
    encodeButtonLayout->addStretch();
    encodeLayout->addLayout(encodeButtonLayout);

    QLabel *encodeOutputLabel = new QLabel("Encoded Result:");
    encodeOutputLabel->setProperty("class", "sectionLabel");
    encodeLayout->addWidget(encodeOutputLabel);

    encodeOutput = new QTextEdit;
    encodeOutput->setProperty("class", "editor output");
    encodeOutput->setReadOnly(true);
    encodeOutput->setFont(fixedFont);
    encodeOutput->setMinimumHeight(150);
    encodeLayout->addWidget(encodeOutput);

    connect(encodeBtn, &QPushButton::clicked, this, &MainWindow::encode);
    connect(copyEncodeBtn, &QPushButton::clicked, this, &MainWindow::copyEncode);
    connect(clearEncodeBtn, &QPushButton::clicked, this, &MainWindow::clearEncode);
    connect(saveFileBtn, &QPushButton::clicked, this, &MainWindow::saveFile);

    tabs->addTab(encodeTab, "Encode");

    // ---------- Pipeline Tab ----------
    QWidget *pipelineTab = new QWidget;
    QVBoxLayout *pipelineLayout = new QVBoxLayout(pipelineTab);
    pipelineLayout->setSpacing(8);  // compact density

    QLabel *pipelineInputLabel = new QLabel("Encoded Text:");
    pipelineInputLabel->setProperty("class", "sectionLabel");
    pipelineLayout->addWidget(pipelineInputLabel);

    pipelineInput = new QTextEdit;
    pipelineInput->setProperty("class", "editor");
    pipelineInput->setPlaceholderText("Enter encoded text to run auto‑pipeline...");
    pipelineInput->setMinimumHeight(120);
    pipelineInput->setFont(fixedFont);
    pipelineLayout->addWidget(pipelineInput);

    QHBoxLayout *pipelineButtonLayout = new QHBoxLayout;
    QPushButton *runPipelineBtn = new QPushButton("Run Pipeline");
    runPipelineBtn->setProperty("class", "primary");
    QPushButton *copyPipelineBtn = new QPushButton("Copy Result");
    copyPipelineBtn->setProperty("class", "secondary");
    pipelineButtonLayout->addWidget(runPipelineBtn);
    pipelineButtonLayout->addWidget(copyPipelineBtn);
    pipelineButtonLayout->addStretch();
    pipelineLayout->addLayout(pipelineButtonLayout);

    pipelineScoreLabel = new QLabel("Score: —");
    pipelineScoreLabel->setProperty("class", "metricLabel");
    pipelineStepsLabel = new QLabel("Flow: Heuristic -> Planner -> Executor -> Scoring -> Retry");
    pipelineStepsLabel->setProperty("class", "metricLabel");
    pipelineLayout->addWidget(pipelineScoreLabel);
    pipelineLayout->addWidget(pipelineStepsLabel);

    QLabel *pipelineOutputLabel = new QLabel("Best Result:");
    pipelineOutputLabel->setProperty("class", "sectionLabel");
    pipelineLayout->addWidget(pipelineOutputLabel);

    pipelineOutput = new QTextEdit;
    pipelineOutput->setProperty("class", "editor output");
    pipelineOutput->setReadOnly(true);
    pipelineOutput->setFont(fixedFont);
    pipelineOutput->setMinimumHeight(150);
    pipelineLayout->addWidget(pipelineOutput);

    connect(runPipelineBtn, &QPushButton::clicked, this, &MainWindow::runPipeline);
    connect(copyPipelineBtn, &QPushButton::clicked, this, &MainWindow::copyPipeline);

    tabs->addTab(pipelineTab, "Pipeline");

    // ---------- Settings Tab ----------
    QWidget *settingsTab = new QWidget;
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsTab);
    settingsLayout->setSpacing(12);  // compact density

    QFrame *themeFrame = new QFrame;
    themeFrame->setFrameShape(QFrame::StyledPanel);
    themeFrame->setProperty("class", "card");
    QHBoxLayout *themeLayout = new QHBoxLayout(themeFrame);
    darkThemeCheck = new QCheckBox("Dark Theme");
    darkThemeCheck->setChecked(true);
    themeLayout->addWidget(darkThemeCheck);
    themeLayout->addStretch();
    settingsLayout->addWidget(themeFrame);

    QFrame *autoFrame = new QFrame;
    autoFrame->setFrameShape(QFrame::StyledPanel);
    autoFrame->setProperty("class", "card");
    QHBoxLayout *autoLayout = new QHBoxLayout(autoFrame);
    autoDecodeCheck = new QCheckBox("Auto-decode on edit");
    autoDecodeCheck->setChecked(false);
    autoLayout->addWidget(autoDecodeCheck);
    autoLayout->addStretch();
    settingsLayout->addWidget(autoFrame);

    QFrame *notifFrame = new QFrame;
    notifFrame->setFrameShape(QFrame::StyledPanel);
    notifFrame->setProperty("class", "card");
    QHBoxLayout *notifLayout = new QHBoxLayout(notifFrame);
    notificationsCheck = new QCheckBox("Show notifications");
    notificationsCheck->setChecked(notificationsEnabled);
    notifLayout->addWidget(notificationsCheck);
    notifLayout->addStretch();
    settingsLayout->addWidget(notifFrame);

    QFrame *buttonFrame = new QFrame;
    buttonFrame->setFrameShape(QFrame::StyledPanel);
    buttonFrame->setProperty("class", "card");
    QHBoxLayout *buttonFrameLayout = new QHBoxLayout(buttonFrame);
    QPushButton *refreshBtn = new QPushButton("Refresh Formats");
    refreshBtn->setProperty("class", "secondary");
    QPushButton *clearCacheBtn = new QPushButton("Clear Cache");
    clearCacheBtn->setProperty("class", "warning");
    buttonFrameLayout->addWidget(refreshBtn);
    buttonFrameLayout->addWidget(clearCacheBtn);
    buttonFrameLayout->addStretch();
    settingsLayout->addWidget(buttonFrame);

    QFrame *aboutFrame = new QFrame;
    aboutFrame->setFrameShape(QFrame::StyledPanel);
    aboutFrame->setProperty("class", "card");
    QHBoxLayout *aboutLayout = new QHBoxLayout(aboutFrame);
    QPushButton *aboutBtn = new QPushButton("About");
    aboutBtn->setProperty("class", "inline");
    aboutLayout->addWidget(aboutBtn);
    aboutLayout->addStretch();
    settingsLayout->addWidget(aboutFrame);

    settingsLayout->addStretch();

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshFormats);
    connect(clearCacheBtn, &QPushButton::clicked, this, &MainWindow::clearCache);
    connect(aboutBtn, &QPushButton::clicked, this, &MainWindow::showAbout);

    tabs->addTab(settingsTab, "Settings");

    // ---------- History Tab ----------
    historyTab = new HistoryTab;
    historyTab->setProperty("class", "card");
    tabs->addTab(historyTab, "History");

    mainLayout->addWidget(tabs);
    setCentralWidget(central);

    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready", 2000);

    setWindowTitle("Auto Decoder Pro");
    resize(1100, 800);
    setMinimumSize(800, 600);
}

void MainWindow::applyTheme(bool dark)
{
    QString stylesheet;
    
    if (dark) {
        // Load master stylesheet and dark theme from resources
        QFile masterFile(":/style/style.qss");
        QFile darkFile(":/style/dark.qss");
        
        if (masterFile.open(QFile::ReadOnly) && darkFile.open(QFile::ReadOnly)) {
            stylesheet = QString::fromUtf8(masterFile.readAll()) + "\n" + 
                        QString::fromUtf8(darkFile.readAll());
            masterFile.close();
            darkFile.close();
        }
    } else {
        // Load master stylesheet and light theme from resources
        QFile masterFile(":/style/style.qss");
        QFile lightFile(":/style/light.qss");
        
        if (masterFile.open(QFile::ReadOnly) && lightFile.open(QFile::ReadOnly)) {
            stylesheet = QString::fromUtf8(masterFile.readAll()) + "\n" + 
                        QString::fromUtf8(lightFile.readAll());
            masterFile.close();
            lightFile.close();
        }
    }
    
    qApp->setStyleSheet(stylesheet);
    updateStatus(dark ? "Dark theme applied" : "Light theme applied");
}

void MainWindow::toggleTheme(bool dark)
{
    isDarkTheme = dark;
    applyTheme(dark);
    savePreferences();
}

void MainWindow::updateStatus(const QString &message, int timeout)
{
    statusBar->showMessage(message, timeout);

    // Check if notifications are enabled before showing
    if (!notificationsEnabled) {
        return;
    }

    // Detect message type and show notification
    NotificationManager::MessageType type = NotificationManager::Info;

    if (message.contains("failed", Qt::CaseInsensitive) ||
        message.contains("error", Qt::CaseInsensitive) ||
        message.contains("cannot", Qt::CaseInsensitive)) {
        type = NotificationManager::Error;
    } else if (message.contains("Encoded to", Qt::CaseSensitive) ||
               message.contains("Decoded in", Qt::CaseSensitive) ||
               message.contains("Clipboard", Qt::CaseSensitive) ||
               message.contains("Cleared", Qt::CaseSensitive) ||
               message.contains("cleared", Qt::CaseSensitive) ||
               message.contains("Refreshed", Qt::CaseSensitive) ||
               message.contains("finished", Qt::CaseInsensitive) ||
               message.contains("Loaded", Qt::CaseSensitive) ||
               message.contains("Saved", Qt::CaseSensitive)) {
        type = NotificationManager::Success;
    } else if (message.contains("Running", Qt::CaseSensitive) ||
               message.contains("Decoding", Qt::CaseSensitive) ||
               message.contains("Encoding", Qt::CaseSensitive)) {
        type = NotificationManager::Info;
    }

    notificationManager.showNotification(message, type, timeout);
}

void MainWindow::decode()
{
    QString input = decodeInput->toPlainText();
    if (input.isEmpty()) {
        decodeOutput->clear();
        detectedFormatLabel->setText("Detected format: —");
        return;
    }

    int idx = decodeFormatCombo->currentIndex();
    updateStatus("Decoding...", 1000);
    auto result = engine.decode(input, idx);

    if (result.success) {
        decodeOutput->setPlainText(result.output);
        detectedFormatLabel->setText("Detected format: " + result.format);
        updateStatus(QString("Decoded in %1 ms").arg(result.processingTimeMs));

        // Record in history
        HistoryManager::instance().addEntry("decode", result.format, input, result.output,
                                            result.processingTimeMs, false);
    } else {
        decodeOutput->setPlainText("Decode failed");
        detectedFormatLabel->setText("Detected format: Error");
        updateStatus(result.errorMessage, 3000);
    }
}

void MainWindow::encode()
{
    QString input = encodeInput->toPlainText();
    if (input.isEmpty()) {
        encodeOutput->clear();
        return;
    }

    int idx = encodeFormatCombo->currentIndex();
    updateStatus("Encoding...", 1000);
    auto result = engine.encode(input, idx);

    if (result.success) {
        encodeOutput->setPlainText(result.output);
        updateStatus("Encoded to " + encodeFormatCombo->currentText());

        // Record in history
        HistoryManager::instance().addEntry("encode", encodeFormatCombo->currentText(), input,
                                            result.output, 0.0, false);
    } else {
        encodeOutput->setPlainText("Encode failed");
        updateStatus(result.errorMessage, 3000);
    }
}

void MainWindow::runPipeline()
{
    QString input = pipelineInput->toPlainText();
    if (input.isEmpty()) {
        updateStatus("Please enter input for pipeline", 2000);
        return;
    }

    updateStatus("Running smart pipeline (heuristic -> planner -> executor -> scoring -> retry)...");
    auto result = engine.runPipeline(input, 3, 5);

    if (result.success) {
        pipelineOutput->setPlainText(result.output);
        pipelineScoreLabel->setText(QString("Score: %1").arg(result.score, 0, 'f', 3));
        pipelineStepsLabel->setText(result.steps);
        updateStatus(QString("Pipeline finished. Score: %1").arg(result.score, 0, 'f', 3));

        // Record in history
        HistoryManager::instance().addEntry("pipeline", result.steps, input, result.output,
                                            0.0, false);
    } else {
        pipelineOutput->setPlainText("Pipeline found nothing");
        pipelineScoreLabel->setText("Score: —");
        pipelineStepsLabel->setText("Flow: Heuristic -> Planner -> Executor -> Scoring -> Retry");
        updateStatus("Pipeline found no result", 3000);
    }
}

void MainWindow::clearDecode()
{
    decodeInput->clear();
    decodeOutput->clear();
    detectedFormatLabel->setText("Detected format: —");
    updateStatus("Cleared decode tab");
}

void MainWindow::clearEncode()
{
    encodeInput->clear();
    encodeOutput->clear();
    updateStatus("Cleared encode tab");
}

void MainWindow::copyDecode()
{
    if (!decodeOutput->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(decodeOutput->toPlainText());
        updateStatus("Copied decode result to clipboard");
    }
}

void MainWindow::copyEncode()
{
    if (!encodeOutput->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(encodeOutput->toPlainText());
        updateStatus("Copied encode result to clipboard");
    }
}

void MainWindow::copyPipeline()
{
    if (!pipelineOutput->toPlainText().isEmpty()) {
        QApplication::clipboard()->setText(pipelineOutput->toPlainText());
        updateStatus("Copied pipeline result to clipboard");
    }
}

void MainWindow::refreshFormats()
{
    engine.refreshFormats();
    decodeFormatCombo->clear();
    decodeFormatCombo->addItems(engine.decodeFormats());
    encodeFormatCombo->clear();
    encodeFormatCombo->addItems(engine.encodeFormats());
    updateStatus("Format lists refreshed");
}

void MainWindow::clearCache()
{
    engine.clearCache();
    updateStatus("Cache cleared");
    QMessageBox::information(this, "Cache", "Cache cleared successfully.");
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open File");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            decodeInput->setPlainText(in.readAll());
            decode();
            updateStatus("Loaded file: " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Cannot open file.");
            updateStatus("Failed to open file", 3000);
        }
    }
}

void MainWindow::saveFile()
{
    if (encodeOutput->toPlainText().isEmpty()) {
        updateStatus("Nothing to save", 2000);
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, "Save Encoded File");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << encodeOutput->toPlainText();
            updateStatus("Saved to: " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Cannot save file.");
            updateStatus("Failed to save file", 3000);
        }
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About Auto Decoder Pro",
        QString("<b>Auto Decoder Pro</b><br/>"
                "Version 2.0.0<br/><br/>"
                "A professional encoding/decoding tool with plugin support.<br/>"
                "Supports: Base64, Hex, Binary, Morse, ROT13, URL, Atbash, Caesar, XOR, Scramble.<br/><br/>"
                "Built with Qt and GLib.<br/>"
                "© 2024 Auto Decoder Team"));
}

void MainWindow::onHistoryItemSelected(const QString &operation, const QString &input)
{
    if (operation.toLower() == "decode") {
        decodeInput->setPlainText(input);
        decode();
    } else if (operation.toLower() == "encode") {
        encodeInput->setPlainText(input);
        encode();
    } else if (operation.toLower() == "pipeline") {
        pipelineInput->setPlainText(input);
        runPipeline();
    }
}

void MainWindow::loadPreferences()
{
    QSettings settings("AutoDecoderPro", "AutoDecoderPro");

    notificationsEnabled = settings.value("notifications_enabled", true).toBool();
    isDarkTheme = settings.value("dark_theme", true).toBool();
    autoDecode = settings.value("auto_decode", false).toBool();

    // Update UI to match loaded preferences
    notificationsCheck->setChecked(notificationsEnabled);
    darkThemeCheck->setChecked(isDarkTheme);
    autoDecodeCheck->setChecked(autoDecode);
}

void MainWindow::savePreferences()
{
    QSettings settings("AutoDecoderPro", "AutoDecoderPro");

    settings.setValue("notifications_enabled", notificationsEnabled);
    settings.setValue("dark_theme", isDarkTheme);
    settings.setValue("auto_decode", autoDecode);
}
