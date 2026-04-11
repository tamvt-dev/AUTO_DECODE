#include "recipe_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonArray>
#include <QApplication>
#include <QGroupBox>
#include "arg_editor_dialog.h"

void RecipeTab::editArgs(QListWidgetItem *item)
{
    QJsonObject op = item->data(Qt::UserRole).toJsonObject();
    ArgEditorDialog dialog(op, this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject updated = op;
        updated["args"] = dialog.getArgs();
        item->setData(Qt::UserRole, updated);
        item->setText(op["name"].toString() + " (args edited)");
    }
}

RecipeTab::RecipeTab(QWidget *parent)
    : QWidget(parent)
    , m_recipeManager(RecipeManager::instance())
    , m_engine(DecoderEngine::instance())
    , m_argEditor(nullptr)
{
    setupUi();
    connect(m_chainOpsList, &QListWidget::itemChanged, this, &RecipeTab::onChainOpsChanged);
}

void RecipeTab::setupUi()
{
    setProperty("class", "recipeTab");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Header
    QLabel *header = new QLabel("Recipe Builder (Drag Ops to Chain)");
    header->setProperty("class", "sectionLabel");
    header->setStyleSheet("font-weight: bold; font-size: 16px;");
    mainLayout->addWidget(header);

    // Split layout for lists using GroupBoxes
    QHBoxLayout *listsLayout = new QHBoxLayout;
    listsLayout->setSpacing(16);

    // Left side: Available Ops Group
    QGroupBox *availGroup = new QGroupBox("Available Operations");
    availGroup->setProperty("class", "card");
    QVBoxLayout *availLayout = new QVBoxLayout(availGroup);

    m_availableOpsList = new QListWidget;
    m_availableOpsList->setFixedWidth(220);
    m_availableOpsList->setMinimumHeight(250);
    m_availableOpsList->setDragEnabled(true);
    m_availableOpsList->setDragDropMode(QAbstractItemView::DragOnly);
    m_availableOpsList->setSelectionMode(QAbstractItemView::SingleSelection);
    availLayout->addWidget(m_availableOpsList);
    listsLayout->addWidget(availGroup);

    // Right side: Chain area Group
    QGroupBox *chainGroup = new QGroupBox("Recipe Chain (Drag to reorder, right-click edit)");
    chainGroup->setProperty("class", "card");
    QVBoxLayout *chainLayout = new QVBoxLayout(chainGroup);

    m_chainOpsList = new QListWidget;
    m_chainOpsList->setAcceptDrops(true);
    m_chainOpsList->setDragEnabled(true);
    m_chainOpsList->setDragDropMode(QAbstractItemView::InternalMove);
    m_chainOpsList->setDropIndicatorShown(true);
    m_chainOpsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_chainOpsList->viewport()->setAcceptDrops(true);
    m_chainOpsList->setMinimumHeight(250);
    chainLayout->addWidget(m_chainOpsList);
    listsLayout->addWidget(chainGroup, 1);

    mainLayout->addLayout(listsLayout);

    // Buttons row
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins(0, 8, 0, 8);
    m_clearBtn = new QPushButton("Clear Chain");
    m_clearBtn->setProperty("class", "secondary");
    m_clearBtn->setMinimumWidth(120);
    connect(m_clearBtn, &QPushButton::clicked, this, &RecipeTab::clearAll);

    m_saveBtn = new QPushButton("Save");
    m_saveBtn->setProperty("class", "secondary");
    connect(m_saveBtn, &QPushButton::clicked, this, &RecipeTab::saveRecipe);

    m_loadBtn = new QPushButton("Load");
    m_loadBtn->setProperty("class", "secondary");
    connect(m_loadBtn, &QPushButton::clicked, this, &RecipeTab::loadRecipe);

    buttonsLayout->addWidget(m_clearBtn);
    buttonsLayout->addWidget(m_saveBtn);
    buttonsLayout->addWidget(m_loadBtn);
    buttonsLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);

    // Input
    QVBoxLayout *inputContainer = new QVBoxLayout;
    QLabel *inputLabel = new QLabel("Input:");
    inputLabel->setProperty("class", "sectionLabel");
    inputContainer->addWidget(inputLabel);

    m_inputEdit = new QTextEdit;
    m_inputEdit->setProperty("class", "editor");
    m_inputEdit->setMinimumHeight(100);
    m_inputEdit->setPlaceholderText("Enter input text for recipe...");
    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_inputEdit->setFont(fixed);
    inputContainer->addWidget(m_inputEdit);

    mainLayout->addLayout(inputContainer);

    // Execute button
    m_executeBtn = new QPushButton("Execute Recipe");
    m_executeBtn->setProperty("class", "primary");
    m_executeBtn->setEnabled(false);
    connect(m_executeBtn, &QPushButton::clicked, this, &RecipeTab::executeRecipe);
    mainLayout->addWidget(m_executeBtn);

    // Output
    QLabel *outputLabel = new QLabel("Output:");
    outputLabel->setProperty("class", "sectionLabel");
    mainLayout->addWidget(outputLabel);

    m_outputEdit = new QTextEdit;
    m_outputEdit->setProperty("class", "editor output");
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setFont(fixed);
    m_outputEdit->setMinimumHeight(120);
    mainLayout->addWidget(m_outputEdit);

    // Populate available ops
    for (const auto& op : m_recipeManager->availableOps()) {
        QListWidgetItem *item = new QListWidgetItem(op["name"].toString());
        item->setData(Qt::UserRole, op);
        m_availableOpsList->addItem(item);
    }

    // Drag-drop setup
    connect(m_availableOpsList, &QListWidget::itemPressed, this, [this](QListWidgetItem *item) {
        if (QApplication::mouseButtons() == Qt::LeftButton) {
            QMimeData *mime = new QMimeData;
            QJsonDocument doc(QJsonObject::fromVariantMap(item->data(Qt::UserRole).toMap()));
            mime->setData("application/x-hyperdecode-op", doc.toJson());
            QDrag *drag = new QDrag(m_availableOpsList);
            drag->setMimeData(mime);
            drag->exec(Qt::CopyAction);
        }
    });

    connect(m_chainOpsList, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        if (auto item = m_chainOpsList->itemAt(pos)) {
            editArgs(item);
        }
    });
}

void RecipeTab::executeRecipe()
{
    QString input = m_inputEdit->toPlainText();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter input text");
        return;
    }

    QJsonArray chainJson = getChainAsJson();
    if (chainJson.isEmpty()) {
        QMessageBox::warning(this, "Error", "Recipe chain is empty");
        return;
    }

    QJsonDocument doc(chainJson);
    auto result = m_engine.executeRecipe(doc.toJson(QJsonDocument::Compact).toStdString().c_str(), input);
    if (result.success) {
        m_outputEdit->setPlainText(result.output);
    } else {
        m_outputEdit->setPlainText("Error: " + result.error);
    }
}

QJsonArray RecipeTab::getChainAsJson() const
{
    QJsonArray array;
    for (int i = 0; i < m_chainOpsList->count(); ++i) {
        auto item = m_chainOpsList->item(i);
        array.append(item->data(Qt::UserRole).toJsonObject());
    }
    return array;
}

void RecipeTab::clearAll()
{
    m_chainOpsList->clear();
    m_inputEdit->clear();
    m_outputEdit->clear();
    updateExecuteButton();
}

void RecipeTab::saveRecipe()
{
    QJsonArray chain = getChainAsJson();
    QJsonObject recipe;
    recipe["name"] = "My Recipe";
    recipe["ops"] = chain;
    m_recipeManager->saveRecipe(recipe);
    QMessageBox::information(this, "Saved", "Recipe saved to recipes.json");
}

void RecipeTab::loadRecipe()
{
    // Simple file dialog for external JSON
    QString fileName = QFileDialog::getOpenFileName(this, "Load Recipe");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isArray()) {
                m_chainOpsList->clear();
                for (const auto& op : doc.array()) {
                    if (op.isObject()) {
                        auto item = new QListWidgetItem(op.toObject()["name"].toString());
                        item->setData(Qt::UserRole, op.toObject());
                        m_chainOpsList->addItem(item);
                    }
                }
                updateExecuteButton();
            }
        }
    }
}

void RecipeTab::onChainOpsChanged()
{
    updateExecuteButton();
}

void RecipeTab::onAvailableOpsDropped()
{
    // Handle drop event for available operations
    updateExecuteButton();
}

void RecipeTab::updateExecuteButton()
{
    m_executeBtn->setEnabled(m_chainOpsList->count() > 0);
}

