#include "recipe_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include "../core/include/core.h"
#include "../core/include/recipe.h"
#include "../core/include/plugin.h"

static RecipeManager* s_instance = nullptr;

RecipeManager* RecipeManager::instance()
{
    if (!s_instance) {
        s_instance = new RecipeManager;
    }
    return s_instance;
}

RecipeManager::RecipeManager() : QObject(nullptr)
{
    loadRecipes();
    loadAvailableOps();
}

QString RecipeManager::recipesPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configDir);
    if (!dir.exists()) dir.mkpath(".");
    return configDir + "/recipes.json";
}

void RecipeManager::loadRecipes()
{
    QFile file(recipesPath());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No recipes file found, starting empty";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        m_recipes.clear();
        for (const QJsonValue &val : array) {
            if (val.isObject()) {
                m_recipes.append(val.toObject());
            }
        }
        emit recipesChanged();
    }
}

void RecipeManager::saveRecipes()
{
    QJsonArray array;
    for (const QJsonObject& recipe : m_recipes) {
        array.append(recipe);
    }
    QJsonDocument doc(array);
    QFile file(recipesPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void RecipeManager::saveRecipe(const QJsonObject &recipe)
{
    m_recipes.append(recipe);
    saveRecipes();
    emit recipesChanged();
}

void RecipeManager::removeRecipe(int index)
{
    if (index >= 0 && index < m_recipes.size()) {
        m_recipes.removeAt(index);
        saveRecipes();
        emit recipesChanged();
    }
}

QList<QJsonObject> RecipeManager::availableOps() const
{
    return m_availableOps;
}

void RecipeManager::loadAvailableOps()
{
    m_availableOps.clear();
    PluginManager* pm = core_get_plugin_manager();
    if (!pm) return;

    GList* plugins = plugin_manager_list(pm);
    for (GList* iter = plugins; iter; iter = iter->next) {
        Plugin* p = static_cast<Plugin*>(iter->data);
        if (p->decode_single) {
            QJsonObject op;
            op["name"] = QString(p->name);
            op["description"] = QString("Decode using %1").arg(p->name);
            op["args"] = QJsonObject();  // Default empty args
            m_availableOps.append(op);
        }
    }
    g_list_free(plugins);
}

Recipe* RecipeManager::toCRrecipe(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    return recipe_parse_json(jsonData.constData());
}

