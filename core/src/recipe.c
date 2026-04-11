#include "../include/recipe.h"
#include "../include/plugin.h"
#include "../include/core.h"
#include <json-glib/json-glib.h>
#include <string.h>

Recipe* recipe_parse_json(const char *json_str) {
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, json_str, -1, NULL)) {
        g_object_unref(parser);
        return NULL;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_ARRAY(root)) {
        g_object_unref(parser);
        return NULL;
    }

    JsonArray *array = json_node_get_array(root);
    Recipe *recipe = NULL;

    for (guint i = 0; i < json_array_get_length(array); i++) {
        JsonObject *op_obj = json_array_get_object_element(array, i);
        RecipeOp *op = g_new0(RecipeOp, 1);
        op->op_name = g_strdup(json_object_get_string_member(op_obj, "op"));
        op->args = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

        JsonObject *args_obj = json_object_get_object_member(op_obj, "args");
        JsonNode *args_node = json_object_get_member(args_obj, "key");
        if (args_node) {
            g_hash_table_insert(op->args, g_strdup("key"), g_strdup(json_node_get_string(args_node)));
        }

        recipe = g_list_append(recipe, op);
    }

    g_object_unref(parser);
    return recipe;
}

void recipe_free(Recipe *recipe) {
    for (GList *iter = recipe; iter; iter = iter->next) {
        RecipeOp *op = iter->data;
        g_free(op->op_name);
        g_hash_table_destroy(op->args);
        g_free(op);
    }
    g_list_free(recipe);
}

Buffer recipe_execute(const Recipe *recipe, Buffer input) {
    Buffer current = buffer_clone(&input);
    PluginManager *pm = core_get_plugin_manager();

    for (GList *iter = (GList*)recipe; iter; iter = iter->next) {
        RecipeOp *op = iter->data;
        Plugin *p = NULL;
        GList *plugins = plugin_manager_list(pm);
        for (GList *piter = plugins; piter; piter = piter->next) {
            Plugin *cand = piter->data;
            if (strcmp(cand->name, op->op_name) == 0) {
                p = cand;
                break;
            }
        }
        g_list_free(plugins);

        if (p && p->decode_single) {
            buffer_free(&current);
            current = p->decode_single(current);
        } else {
            buffer_free(&current);
            Buffer fail = {NULL, 0};
            return fail;
        }
    }
    return current;
}

gboolean recipe_save(const Recipe *recipe __attribute__((unused)), const char *path __attribute__((unused))) {
    // Stub: save JSON
    return TRUE;
}

Recipe* recipe_load(const char *path __attribute__((unused))) {
    // Stub: load JSON
    return NULL;
}

