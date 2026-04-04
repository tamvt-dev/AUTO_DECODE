#ifndef PLUGIN_H
#define PLUGIN_H

#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "buffer.h"

// Plugin function types
typedef Buffer (*PluginDecodeSingleFunc)(Buffer in);
typedef GList* (*PluginDecodeMultiFunc)(Buffer in);   // trả về danh sách Buffer*
typedef Buffer (*PluginEncodeFunc)(Buffer in);
typedef gboolean (*PluginDetectFunc)(Buffer in);

// Plugin structure
typedef struct Plugin {
    char *name;
    PluginDecodeSingleFunc decode_single;
    PluginDecodeMultiFunc decode_multi;
    PluginEncodeFunc encode;
    PluginDetectFunc detect;
    int priority;
    gboolean enabled;
    char *meta_info;    // có thể được cập nhật trong quá trình decode (ví dụ key, seed)
} Plugin;

typedef struct PluginManager PluginManager;

PluginManager* plugin_manager_new(void);
void plugin_manager_free(PluginManager *pm);
void plugin_manager_register(PluginManager *pm, Plugin *plugin);
Plugin* plugin_manager_get_for_input(PluginManager *pm, Buffer in);
GList* plugin_manager_list(PluginManager *pm);
void plugin_manager_enable(PluginManager *pm, const char *name, gboolean enable);

// Registry cho plugin tích hợp sẵn
typedef struct {
    void (*register_plugin)(const char *name,
                            PluginDecodeSingleFunc decode_single,
                            PluginDecodeMultiFunc decode_multi,
                            PluginEncodeFunc encode,
                            PluginDetectFunc detect,
                            int priority);
} PluginRegistry;

extern PluginRegistry *g_plugin_registry;

#ifdef __cplusplus
}
#endif
#endif