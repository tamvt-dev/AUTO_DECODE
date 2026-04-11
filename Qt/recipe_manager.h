#ifndef RECIPE_MANAGER_H
#define RECIPE_MANAGER_H

#include <QObject>
#include <QList>
#include <QJsonObject>
#include "../core/include/recipe.h"

class RecipeManager : public QObject {
    Q_OBJECT

public:
    static RecipeManager* instance();
    QList<QJsonObject> recipes() const { return m_recipes; }
    void loadRecipes();
    void saveRecipe(const QJsonObject &recipe);
    void removeRecipe(int index);
    Recipe* toCRrecipe(const QJsonObject &json);
    QList<QJsonObject> availableOps() const;

signals:
    void recipesChanged();

private:
    RecipeManager();
    QString recipesPath() const;
    void saveRecipes();
    void loadAvailableOps();
    
    QList<QJsonObject> m_recipes;
    QList<QJsonObject> m_availableOps;
};

#endif
