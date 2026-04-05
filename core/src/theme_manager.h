#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H


typedef enum {
    THEME_DARK,
    THEME_LIGHT,
    THEME_SYSTEM
} ThemeType;

void theme_manager_init(void);
void theme_manager_apply(void *widget, ThemeType theme);
void theme_manager_toggle(void *widget);
ThemeType theme_manager_get_current(void);
void theme_manager_save(void);
void theme_manager_save_with_value(ThemeType theme);
void theme_manager_load(void);

#endif
