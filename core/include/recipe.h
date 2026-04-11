#ifndef RECIPE_H
#define RECIPE_H

#include <glib.h>
#include "plugin.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *op_name;
    GHashTable *args; // key-value for params (e.g., "key":"1234")
} RecipeOp;

typedef GList Recipe; // GList* RecipeOp*

Recipe* recipe_parse_json(const char *json_str);
void recipe_free(Recipe *recipe);
Buffer recipe_execute(const Recipe *recipe, Buffer input);
gboolean recipe_save(const Recipe *recipe, const char *path);
Recipe* recipe_load(const char *path);

#ifdef __cplusplus
}
#endif

#endif
