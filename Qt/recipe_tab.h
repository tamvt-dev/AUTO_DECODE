#ifndef RECIPE_TAB_H
#define RECIPE_TAB_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QJsonObject>
#include "recipe_manager.h"
#include "decoder_engine.h"

class ArgEditorDialog;

class RecipeTab : public QWidget
{
    Q_OBJECT

public:
    explicit RecipeTab(QWidget *parent = nullptr);

private slots:
    void executeRecipe();
    void clearAll();
    void saveRecipe();
    void loadRecipe();
    void editArgs(QListWidgetItem *item);
    void onAvailableOpsDropped();
    void onChainOpsChanged();

private:
    void setupUi();
    QJsonArray getChainAsJson() const;
    void updateExecuteButton();

    RecipeManager *m_recipeManager;
    DecoderEngine &m_engine;

    QListWidget *m_availableOpsList;
    QListWidget *m_chainOpsList;
    QTextEdit *m_inputEdit;
    QTextEdit *m_outputEdit;
    QPushButton *m_executeBtn;
    QPushButton *m_clearBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_loadBtn;

    ArgEditorDialog *m_argEditor;
};

#endif // RECIPE_TAB_H

