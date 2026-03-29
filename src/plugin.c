#include "../include/plugin.h"
#include "../include/logging.h"

static GHashTable *builtin_plugins = NULL;
static PluginManager *current_manager = NULL;  // current active manager

static void plugin_free(Plugin *p) {
    if (p) { g_free(p->name); g_free(p); }
}

static void register_plugin_builtin(const char *name, PluginDecodeFunc decode, PluginEncodeFunc encode, PluginDetectFunc detect, int priority) {
    Plugin *p = g_new0(Plugin, 1);
    p->name = g_strdup(name);
    p->decode = decode;
    p->encode = encode;
    p->detect = detect;
    p->priority = priority;
    p->enabled = TRUE;

    // If there's an active plugin manager, add directly to it
    if (current_manager) {
        g_mutex_lock(&current_manager->mutex);
        g_hash_table_insert(current_manager->plugins, g_strdup(p->name), p);
        current_manager->load_order = g_list_append(current_manager->load_order, p);
        g_mutex_unlock(&current_manager->mutex);
        log_info("Registered built-in plugin: %s", name);
        return;
    }

    // Fallback: store in builtin_plugins (for compatibility)
    if (!builtin_plugins) builtin_plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)plugin_free);
    g_hash_table_insert(builtin_plugins, g_strdup(p->name), p);
    log_info("Registered built-in plugin (fallback): %s", name);
}

PluginRegistry *g_plugin_registry = &(PluginRegistry){.register_plugin = register_plugin_builtin};

struct PluginManager {
    GHashTable *plugins;
    GList *load_order;
    GMutex mutex;
};

PluginManager* plugin_manager_new(void) {
    PluginManager *pm = g_new0(PluginManager, 1);
    pm->plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)plugin_free);
    g_mutex_init(&pm->mutex);
    current_manager = pm;  // ✅ BỎ COMMENT DÒNG NÀY
    return pm;
}

void plugin_manager_free(PluginManager *pm) {
    if (!pm) return;
    if (pm == current_manager) current_manager = NULL;
    g_mutex_lock(&pm->mutex);
    g_hash_table_destroy(pm->plugins);
    g_list_free(pm->load_order);
    g_mutex_unlock(&pm->mutex);
    g_mutex_clear(&pm->mutex);
    g_free(pm);
}

void plugin_manager_register(PluginManager *pm, Plugin *p) {
    if (!pm || !p) return;
    g_mutex_lock(&pm->mutex);
    g_hash_table_insert(pm->plugins, g_strdup(p->name), p);
    pm->load_order = g_list_append(pm->load_order, p);
    g_mutex_unlock(&pm->mutex);
}

Plugin* plugin_manager_get_for_input(PluginManager *pm, const char *input) {
    if (!pm || !input) return NULL;
    g_mutex_lock(&pm->mutex);
    GList *iter;
    for (iter = pm->load_order; iter; iter = iter->next) {
        Plugin *p = (Plugin*)iter->data;
        if (p->enabled && p->detect && p->detect(input)) {
            g_mutex_unlock(&pm->mutex);
            return p;
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

void plugin_manager_enable(PluginManager *pm, const char *name, gboolean enable) {
    if (!pm || !name) return;
    g_mutex_lock(&pm->mutex);
    Plugin *p = g_hash_table_lookup(pm->plugins, name);
    if (p) p->enabled = enable;
    g_mutex_unlock(&pm->mutex);
}