#ifndef PLUGIN_H
#define PLUGIN_H

#include <glib.h>

typedef struct Plugin Plugin;

typedef char* (*PluginDecodeFunc)(const char *input);
typedef char* (*PluginEncodeFunc)(const char *input);
typedef gboolean (*PluginDetectFunc)(const char *input);
typedef void (*PluginInitFunc)(void);
typedef void (*PluginCleanupFunc)(void);

struct Plugin {
    char *name;
    char *version;
    char *author;
    char *description;
    void *handle;
    
    PluginDecodeFunc decode;
    PluginEncodeFunc encode;
    PluginDetectFunc detect;
    PluginInitFunc init;
    PluginCleanupFunc cleanup;
    
    gboolean loaded;
    gboolean enabled;
    int priority;
};

typedef struct PluginManager PluginManager;

PluginManager* plugin_manager_new(const char *plugins_dir);
void plugin_manager_free(PluginManager *pm);
gboolean plugin_manager_load_all(PluginManager *pm);
gboolean plugin_manager_load(PluginManager *pm, const char *plugin_path);
Plugin* plugin_manager_get_for_input(PluginManager *pm, const char *input);
GList* plugin_manager_list(PluginManager *pm);

typedef struct {
    void (*register_plugin)(const char *name, 
                            PluginDecodeFunc decode,
                            PluginEncodeFunc encode,
                            PluginDetectFunc detect,
                            int priority);
} PluginRegistry;

extern PluginRegistry *g_plugin_registry;

#endif /* PLUGIN_H */