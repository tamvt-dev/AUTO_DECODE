#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <gtk/gtk.h>

typedef enum {
    THEME_DARK,
    THEME_LIGHT,
    THEME_SYSTEM
} ThemeType;

void theme_manager_init(void);
void theme_manager_apply(GtkWidget *widget, ThemeType theme);
void theme_manager_toggle(GtkWidget *widget);
ThemeType theme_manager_get_current(void);
void theme_manager_save(void);
void theme_manager_load(void);

#endif