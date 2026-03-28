#include "../include/plugin.h"
#include "../include/logging.h"
#include <gmodule.h>

static GHashTable *builtin_plugins = NULL;
static GMutex builtin_mutex;

static void register_plugin_builtin(const char *name,
                                     PluginDecodeFunc decode,
                                     PluginEncodeFunc encode,
                                     PluginDetectFunc detect,
                                     int priority) {
    g_mutex_lock(&builtin_mutex);
    
    if (!builtin_plugins) {
        builtin_plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    }
    
    Plugin *plugin = g_new0(Plugin, 1);
    plugin->name = g_strdup(name);
    plugin->decode = decode;
    plugin->encode = encode;
    plugin->detect = detect;
    plugin->loaded = TRUE;
    plugin->enabled = TRUE;
    plugin->priority = priority;
    
    g_hash_table_insert(builtin_plugins, plugin->name, plugin);
    log_info("Registered built-in plugin: %s", name);
    
    g_mutex_unlock(&builtin_mutex);
}

PluginRegistry *g_plugin_registry = &(PluginRegistry){.register_plugin = register_plugin_builtin};

struct PluginManager {
    GHashTable *plugins;
    GList *load_order;
    char *plugins_dir;
    GMutex mutex;
};

static void plugin_free(Plugin *plugin) {
    if (plugin) {
        if (plugin->cleanup) plugin->cleanup();
        if (plugin->handle) g_module_close(plugin->handle);
        g_free(plugin->name);
        g_free(plugin->version);
        g_free(plugin->author);
        g_free(plugin->description);
        g_free(plugin);
    }
}

PluginManager* plugin_manager_new(const char *plugins_dir) {
    PluginManager *pm = g_new0(PluginManager, 1);
    pm->plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)plugin_free);
    pm->plugins_dir = g_strdup(plugins_dir);
    g_mutex_init(&pm->mutex);
    return pm;
}

void plugin_manager_free(PluginManager *pm) {
    if (pm) {
        g_mutex_lock(&pm->mutex);
        g_hash_table_destroy(pm->plugins);
        g_list_free(pm->load_order);
        g_mutex_unlock(&pm->mutex);
        g_mutex_clear(&pm->mutex);
        g_free(pm->plugins_dir);
        g_free(pm);
    }
}

gboolean plugin_manager_load(PluginManager *pm, const char *plugin_path) {
    if (!pm || !plugin_path) return FALSE;
    
    g_mutex_lock(&pm->mutex);
    
    GModule *module = g_module_open(plugin_path, G_MODULE_BIND_LAZY);
    if (!module) {
        log_error("Failed to load plugin: %s", g_module_error());
        g_mutex_unlock(&pm->mutex);
        return FALSE;
    }
    
    Plugin *plugin = g_new0(Plugin, 1);
    plugin->name = g_path_get_basename(plugin_path);
    plugin->handle = module;
    
    g_module_symbol(module, "plugin_init", (gpointer*)&plugin->init);
    g_module_symbol(module, "plugin_cleanup", (gpointer*)&plugin->cleanup);
    g_module_symbol(module, "plugin_decode", (gpointer*)&plugin->decode);
    g_module_symbol(module, "plugin_encode", (gpointer*)&plugin->encode);
    g_module_symbol(module, "plugin_detect", (gpointer*)&plugin->detect);
    
    if (plugin->init) plugin->init();
    
    plugin->loaded = TRUE;
    plugin->enabled = TRUE;
    
    g_hash_table_insert(pm->plugins, g_strdup(plugin->name), plugin);
    pm->load_order = g_list_append(pm->load_order, plugin);
    
    log_info("Loaded plugin: %s", plugin->name);
    g_mutex_unlock(&pm->mutex);
    
    return TRUE;
}

gboolean plugin_manager_load_all(PluginManager *pm) {
    if (!pm || !pm->plugins_dir) return FALSE;
    
    GDir *dir = g_dir_open(pm->plugins_dir, 0, NULL);
    if (dir) {
        const char *name;
        while ((name = g_dir_read_name(dir))) {
            if (g_str_has_suffix(name, ".so") || 
                g_str_has_suffix(name, ".dll") ||
                g_str_has_suffix(name, ".dylib")) {
                char *path = g_build_filename(pm->plugins_dir, name, NULL);
                plugin_manager_load(pm, path);
                g_free(path);
            }
        }
        g_dir_close(dir);
    }
    
    g_mutex_lock(&builtin_mutex);
    if (builtin_plugins) {
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, builtin_plugins);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            Plugin *plugin = (Plugin*)value;
            if (!g_hash_table_contains(pm->plugins, plugin->name)) {
                g_hash_table_insert(pm->plugins, g_strdup(plugin->name), plugin);
                pm->load_order = g_list_append(pm->load_order, plugin);
            }
        }
    }
    g_mutex_unlock(&builtin_mutex);
    
    return TRUE;
}

Plugin* plugin_manager_get_for_input(PluginManager *pm, const char *input) {
    if (!pm || !input) return NULL;
    
    g_mutex_lock(&pm->mutex);
    
    GList *iter;
    for (iter = pm->load_order; iter; iter = iter->next) {
        Plugin *plugin = (Plugin*)iter->data;
        if (plugin->enabled && plugin->detect && plugin->detect(input)) {
            g_mutex_unlock(&pm->mutex);
            return plugin;
        }
    }
    
    g_mutex_unlock(&pm->mutex);
    return NULL;
}

GList* plugin_manager_list(PluginManager *pm) {
    if (!pm) return NULL;
    
    g_mutex_lock(&pm->mutex);
    GList *list = g_list_copy(pm->load_order);
    g_mutex_unlock(&pm->mutex);
    
    return list;
}