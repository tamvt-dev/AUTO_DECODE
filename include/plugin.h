#ifndef PLUGIN_H
#define PLUGIN_H

#include <glib.h>

typedef char* (*PluginDecodeFunc)(const char *input);
typedef char* (*PluginEncodeFunc)(const char *input);
typedef gboolean (*PluginDetectFunc)(const char *input);

typedef struct Plugin {
    char *name;
    PluginDecodeFunc decode;
    PluginEncodeFunc encode;
    PluginDetectFunc detect;
    int priority;
    gboolean enabled;
} Plugin;

typedef struct PluginManager {
    GHashTable *plugins;
    GList *load_order;
    GMutex mutex;
} PluginManager;

PluginManager* plugin_manager_new(void);
void plugin_manager_free(PluginManager *pm);
void plugin_manager_register(PluginManager *pm, Plugin *plugin);
Plugin* plugin_manager_get_for_input(PluginManager *pm, const char *input);
GList* plugin_manager_list(PluginManager *pm);
void plugin_manager_enable(PluginManager *pm, const char *name, gboolean enable);

// Registry to allow plugins to register themselves
typedef struct {
    void (*register_plugin)(const char *name,
                            PluginDecodeFunc decode,
                            PluginEncodeFunc encode,
                            PluginDetectFunc detect,
                            int priority);
} PluginRegistry;

extern PluginRegistry *g_plugin_registry;

#endif