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
#include <QSignalBlocker>
#include <QPixmap>
#include <Qt>
#include <QtConcurrent>
#include <QListWidget>
#include <QScrollArea>
#include <QGroupBox>
#include "console_manager.h"
#include "history_manager.h"
#include "history_tab.h"
#include "decoder_engine.h"
#include "recipe_manager.h"
#include "notification_manager.h"
#include "batch_processor.h"
#include "recipe_tab.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , engine(DecoderEngine::instance())
    , recipeManager(RecipeManager::instance())
    , notificationManager(NotificationManager::instance())
    , batchProcessor(new BatchProcessor())
    , autoDecode(false)
    , isDarkTheme(true)
    , notificationsEnabled(true)
    , hexViewEnabled(false)
{
    setupUi();
    preloadThemes();

    // Initialize notification manager
    notificationManager.setParentWidget(this);

    // Set batch processor parent for proper cleanup
    batchProcessor->setParent(this);

    // Connect batch processor signals
    connect(batchProcessor, &BatchProcessor::progressUpdated, this, &MainWindow::onBatchProgress);
    connect(batchProcessor, &BatchProcessor::statusUpdated, this, &MainWindow::onBatchStatus);
    connect(batchProcessor, &BatchProcessor::finished, this, &MainWindow::onBatchFinished);

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
    connect(showConsoleCheck, &QCheckBox::toggled, this, &MainWindow::toggleConsole);
    connect(historyTab, &HistoryTab::itemSelected, this, &MainWindow::onHistoryItemSelected);

    // Hex view toggle
    connect(hexViewCheck, &QCheckBox::toggled, this, &MainWindow::toggleHexView);

    // Batch connections (connected in setupUi)
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

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(12);
    headerLayout->setContentsMargins(0, 0, 0, 4);

    QLabel *logoLabel = new QLabel;
    logoLabel->setProperty("class", "appLogo");
    QPixmap logoPixmap(":/branding/hyperdecode-mark.svg");
    logoLabel->setPixmap(logoPixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setFixedSize(40, 40);
    logoLabel->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("HYPERDECODE");
    titleLabel->setProperty("class", "appTitle");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; letter-spacing: 1px; color: #37F712;");

    headerLayout->addWidget(logoLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

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

    QHBoxLayout *selectionLayout = new QHBoxLayout;
    QLabel *listLabel = new QLabel("Top Candidates:");
    listLabel->setProperty("class", "sectionLabel");
    selectionLayout->addWidget(listLabel);

    candidateCombo = new QComboBox;
    candidateCombo->setProperty("class", "candidateCombo");
    candidateCombo->setMinimumWidth(350);
    selectionLayout->addWidget(candidateCombo);
    selectionLayout->addStretch();
    pipelineLayout->addLayout(selectionLayout);

    // Detail section below selection
    QVBoxLayout *detailSide = new QVBoxLayout;
    detailSide->setSpacing(12);

    QLabel *flowLabel = new QLabel("Transformation Flow (Click step to inspect):");
    flowLabel->setProperty("class", "sectionLabel");
    detailSide->addWidget(flowLabel);

    QScrollArea *flowScroll = new QScrollArea;
    flowScroll->setWidgetResizable(true);
    flowScroll->setFixedHeight(60);
    flowScroll->setFrameShape(QFrame::NoFrame);
    historyFlowWidget = new QWidget;
    historyFlowWidget->setLayout(new QHBoxLayout);
    historyFlowWidget->layout()->setContentsMargins(0, 0, 0, 0);
    flowScroll->setWidget(historyFlowWidget);
    detailSide->addWidget(flowScroll);

    QLabel *pipelineOutputLabel = new QLabel("Output Inspection:");
    pipelineOutputLabel->setProperty("class", "sectionLabel");
    detailSide->addWidget(pipelineOutputLabel);

    pipelineOutput = new QTextEdit;
    pipelineOutput->setProperty("class", "editor output");
    pipelineOutput->setReadOnly(true);
    pipelineOutput->setFont(fixedFont);
    pipelineOutput->setMinimumHeight(120);
    detailSide->addWidget(pipelineOutput);

    pipelineLayout->addLayout(detailSide);

    connect(runPipelineBtn, &QPushButton::clicked, this, &MainWindow::runPipeline);
    connect(copyPipelineBtn, &QPushButton::clicked, this, &MainWindow::copyPipeline);
    connect(candidateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCandidateSelected);

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

    QFrame *consoleFrame = new QFrame;
    consoleFrame->setFrameShape(QFrame::StyledPanel);
    consoleFrame->setProperty("class", "card");
    QHBoxLayout *consoleLayout = new QHBoxLayout(consoleFrame);
    showConsoleCheck = new QCheckBox("Show command window");
    showConsoleCheck->setChecked(false);
    consoleLayout->addWidget(showConsoleCheck);
    consoleLayout->addStretch();
    settingsLayout->addWidget(consoleFrame);

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

    // ---------- Batch Tab ----------
    QWidget *batchTab = new QWidget;
    QVBoxLayout *batchLayout = new QVBoxLayout(batchTab);
    batchLayout->setSpacing(8);

    QHBoxLayout *inputDirLayout = new QHBoxLayout;
    QLabel *inputDirLabel = new QLabel("Input Dir:");
    batchInputDir = new QLineEdit;
    QPushButton *browseInputBtn = new QPushButton("Browse");
    inputDirLayout->addWidget(inputDirLabel);
    inputDirLayout->addWidget(batchInputDir);
    inputDirLayout->addWidget(browseInputBtn);
    batchLayout->addLayout(inputDirLayout);

    QHBoxLayout *outputDirLayout = new QHBoxLayout;
    QLabel *outputDirLabel = new QLabel("Output Dir:");
    batchOutputDir = new QLineEdit;
    QPushButton *browseOutputBtn = new QPushButton("Browse");
    outputDirLayout->addWidget(outputDirLabel);
    outputDirLayout->addWidget(batchOutputDir);
    outputDirLayout->addWidget(browseOutputBtn);
    batchLayout->addLayout(outputDirLayout);

    QLabel *recipeLabel = new QLabel("Recipe JSON:");
    batchRecipeJson = new QTextEdit;
    batchRecipeJson->setPlaceholderText("{\"steps\": [{\"name\": \"base64_decode\"}, ...]}");
    batchRecipeJson->setMinimumHeight(100);
    batchLayout->addWidget(recipeLabel);
    batchLayout->addWidget(batchRecipeJson);

    QHBoxLayout *batchButtonLayout = new QHBoxLayout;
    QPushButton *startBatchBtn = new QPushButton("Start Batch");
    startBatchBtn->setProperty("class", "primary");
    QPushButton *clearBatchLogBtn = new QPushButton("Clear Log");
    batchButtonLayout->addWidget(startBatchBtn);
    batchButtonLayout->addWidget(clearBatchLogBtn);
    batchButtonLayout->addStretch();
    batchLayout->addLayout(batchButtonLayout);

    batchProgressBar = new QProgressBar;
    batchProgressBar->setRange(0, 100);
    batchProgressBar->setValue(0);
    batchLayout->addWidget(batchProgressBar);

    batchLog = new QTextEdit;
    batchLog->setReadOnly(true);
    batchLog->setMaximumHeight(150);
    batchLayout->addWidget(batchLog);

    connect(startBatchBtn, &QPushButton::clicked, this, &MainWindow::startBatch);
    connect(clearBatchLogBtn, &QPushButton::clicked, [this]() { batchLog->clear(); });
    connect(browseInputBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Input Directory");
        if (!dir.isEmpty()) batchInputDir->setText(dir);
    });
    connect(browseOutputBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
        if (!dir.isEmpty()) batchOutputDir->setText(dir);
    });

    // Batch processor signals (static for now)
    // connect for signals when making instance-based

    // Hex view checkbox in Settings tab? Add to settings if needed
    hexViewCheck = new QCheckBox("Hex View Outputs");
    settingsLayout->insertWidget(1, hexViewCheck);  // After theme

    tabs->addTab(batchTab, "Batch");

    // ---------- Recipe Tab ----------
    recipeTab = new RecipeTab;
    recipeTab->setProperty("class", "card");
    tabs->addTab(recipeTab, "Recipe");

    mainLayout->addWidget(tabs);
    setCentralWidget(central);

    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready", 2000);

    setWindowTitle("HyperDecode");
    resize(1100, 800);
    setMinimumSize(800, 600);
}

QString MainWindow::loadStylesheetFromResource(const QString &resourcePath) const
{
    QFile file(resourcePath);
    if (!file.open(QFile::ReadOnly)) {
        return QString();
    }

    const QString stylesheet = QString::fromUtf8(file.readAll());
    file.close();
    return stylesheet;
}

void MainWindow::preloadThemes()
{
    const QString sharedStylesheet = loadStylesheetFromResource(":/style/style.qss");
    darkStylesheet = sharedStylesheet + "\n" + loadStylesheetFromResource(":/style/dark.qss");
    lightStylesheet = sharedStylesheet + "\n" + loadStylesheetFromResource(":/style/light.qss");
}

void MainWindow::applyTheme(bool dark)
{
    const QString &stylesheet = dark ? darkStylesheet : lightStylesheet;
    if (stylesheet.isEmpty()) {
        statusBar->showMessage("Theme stylesheet could not be loaded", 3000);
        return;
    }

    if (this->styleSheet() == stylesheet) {
        return;
    }

    setUpdatesEnabled(false);
    setProperty("theme", dark ? "dark" : "light");
    this->setStyleSheet(stylesheet);
    setUpdatesEnabled(true);
    update();
}

void MainWindow::toggleTheme(bool dark)
{
    isDarkTheme = dark;
    applyTheme(dark);
    savePreferences();
    statusBar->showMessage(dark ? "Dark theme applied" : "Light theme applied", 2000);
}

void MainWindow::toggleConsole(bool visible)
{
    ConsoleManager::setVisible(visible);
    savePreferences();
    updateStatus(visible
        ? "Command window shown. Logs continue in lastlog.txt"
        : "Command window hidden. Logs continue in lastlog.txt");
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

    updateStatus("Running smart pipeline (Adaptive Beam Search + Diversity)...");
    
    // Disable UI
    candidateCombo->clear();
    currentCandidates.clear();
    pipelineOutput->clear();

    // Run pipeline in background
    (void)QtConcurrent::run([this, input]() {
        auto result = engine.runPipeline(input, 6, 12); // Deep search
        
        QMetaObject::invokeMethod(this, [this, result]() {
            if (result.success && !result.candidates.isEmpty()) {
                currentCandidates = result.candidates;
                
                QSignalBlocker blocker(candidateCombo);
                for (const auto &cand : currentCandidates) {
                    QString label = QString("[%1] %2").arg(QString::number(cand.score, 'f', 2), cand.steps.left(50) + "...");
                    candidateCombo->addItem(label);
                }
                
                candidateCombo->setCurrentIndex(0);
                onCandidateSelected(0); // Manually trigger since blocked
                updateStatus(QString("Pipeline found %1 candidates").arg(currentCandidates.size()));
            } else {
                pipelineOutput->setPlainText("Pipeline discovered no valid paths.");
                updateStatus(result.errorMessage, 3000);
            }
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onCandidateSelected(int index)
{
    if (index < 0 || index >= currentCandidates.size()) return;

    const auto &cand = currentCandidates[index];
    pipelineOutput->setPlainText(cand.output);
    pipelineScoreLabel->setText(QString("Score: %1").arg(cand.score, 0, 'f', 3));
    pipelineStepsLabel->setText("Route: " + cand.steps);

    // Rebuild Flow Buttons
    QLayoutItem *item;
    while ((item = historyFlowWidget->layout()->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    for (const auto &step : cand.history) {
        QPushButton *stepBtn = new QPushButton(step.first);
        stepBtn->setProperty("class", "stepButton");
        connect(stepBtn, &QPushButton::clicked, [this, step]() {
            onHistoryStepClicked(step.first, step.second);
        });
        historyFlowWidget->layout()->addWidget(stepBtn);
        
        QLabel *arrow = new QLabel("→");
        arrow->setProperty("class", "flowArrow");
        historyFlowWidget->layout()->addWidget(arrow);
    }
    // Remove last arrow
    if (historyFlowWidget->layout()->count() > 0) {
        QLayoutItem *last = historyFlowWidget->layout()->takeAt(historyFlowWidget->layout()->count() - 1);
        if (last->widget()) delete last->widget();
        delete last;
    }
    ((QHBoxLayout*)historyFlowWidget->layout())->addStretch();
}

void MainWindow::onHistoryStepClicked(const QString &name, const QString &output)
{
    pipelineOutput->setPlainText(output);
    updateStatus("Inspecting step: " + name);
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
    QMessageBox about(this);
    about.setWindowTitle("About HyperDecode");
    about.setTextFormat(Qt::RichText);

    QPixmap logoPixmap(":/branding/hyperdecode-mark.svg");
    about.setIconPixmap(logoPixmap.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    about.setText(QString("<b>HyperDecode</b><br/>"
                          "Version 2.0.0<br/><br/>"
                          "A professional encoding/decoding tool with plugin support.<br/>"
                          "Supports: Base64, Hex, Binary, Morse, ROT13, URL, Atbash, Caesar, XOR, Scramble.<br/><br/>"
                          "Built with Qt and GLib.<br/>"
                          "(C) 2026 HyperDecode Team"));
    about.exec();
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
    QSettings settings("HyperDecode", "HyperDecode");

    notificationsEnabled = settings.value("notifications_enabled", true).toBool();
    isDarkTheme = settings.value("dark_theme", true).toBool();
    autoDecode = settings.value("auto_decode", false).toBool();
    bool showConsole = settings.value("show_console", false).toBool();
    hexViewEnabled = settings.value("hex_view", false).toBool();

    // Update UI to match loaded preferences
    QSignalBlocker notifBlocker(notificationsCheck);
    QSignalBlocker themeBlocker(darkThemeCheck);
    QSignalBlocker autoBlocker(autoDecodeCheck);
    QSignalBlocker consoleBlocker(showConsoleCheck);
    QSignalBlocker hexBlocker(hexViewCheck);

    notificationsCheck->setChecked(notificationsEnabled);
    darkThemeCheck->setChecked(isDarkTheme);
    autoDecodeCheck->setChecked(autoDecode);
    showConsoleCheck->setChecked(showConsole);
    hexViewCheck->setChecked(hexViewEnabled);
    ConsoleManager::setVisible(showConsole);
}

void MainWindow::savePreferences()
{
    QSettings settings("HyperDecode", "HyperDecode");

    settings.setValue("notifications_enabled", notificationsEnabled);
    settings.setValue("dark_theme", isDarkTheme);
    settings.setValue("auto_decode", autoDecode);
    settings.setValue("show_console", showConsoleCheck->isChecked());
    settings.setValue("hex_view", hexViewEnabled);
}

void MainWindow::startBatch()
{
    QString inputDir = batchInputDir->text();
    QString outputDir = batchOutputDir->text();
    QString recipe = batchRecipeJson->toPlainText();

    if (inputDir.isEmpty() || outputDir.isEmpty() || recipe.isEmpty()) {
        updateStatus("Please fill input dir, output dir, and recipe JSON", 3000);
        return;
    }

    batchProgressBar->setValue(0);
    batchLog->append("Starting batch process...");
    batchProcessor->processDir(inputDir, recipe, outputDir);
    // Note: Signals connected in constructor comment; static method currently, consider instance later
}

void MainWindow::onBatchProgress(int percent)
{
    batchProgressBar->setValue(percent);
}

void MainWindow::onBatchStatus(const QString &message)
{
    batchLog->append(message);
    updateStatus(message);
}

void MainWindow::onBatchFinished(const QString &summary)
{
    batchLog->append("Batch complete: " + summary);
    updateStatus(summary, 5000);
}

QString MainWindow::toHexView(const QString &text)
{
    QByteArray bytes = text.toUtf8();
    QString hex;
    for (int i = 0; i < bytes.size(); ++i) {
        hex += QString("%1 ").arg((unsigned char)bytes[i], 2, 16, QChar('0'));
        if ((i + 1) % 16 == 0) hex += "\n";
    }
    return hex;
}

void MainWindow::toggleHexView(bool enabled)
{
    hexViewEnabled = enabled;
    QString decodeText = decodeOutput->toPlainText();
    QString encodeText = encodeOutput->toPlainText();
    QString pipelineText = pipelineOutput->toPlainText();

    if (enabled) {
        decodeOutput->setPlainText(toHexView(decodeText));
        encodeOutput->setPlainText(toHexView(encodeText));
        pipelineOutput->setPlainText(toHexView(pipelineText));
    } else {
        // Revert not stored; simple toggle assumes user ok with loss or re-run
        updateStatus("Hex view toggled; re-run ops to refresh text view");
    }
    savePreferences();
}

