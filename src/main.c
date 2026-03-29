#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "../include/core.h"
#include "../include/version.h"
#include "../include/logging.h"
#include "../include/crash_handler.h"
#include "../include/theme_manager.h"

typedef struct {
    GtkEntry *entry;
    GtkTextView *output;
    GtkLabel *format;
    GtkStatusbar *status;
    GtkComboBoxText *format_combo;
} DecodeContext;

typedef struct {
    GtkEntry *entry;
    GtkTextView *output;
    GtkComboBoxText *combo;
    GtkStatusbar *status;
} EncodeContext;

// Decode callback
static void on_decode_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    DecodeContext *ctx = (DecodeContext*)data;
    const char *input = gtk_entry_get_text(ctx->entry);
    if (strlen(input) == 0) {
        gtk_statusbar_push(ctx->status, 0, "Please enter data");
        return;
    }

    int format_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->format_combo));
    CoreMode mode;

    if (format_idx == 0) {
        mode = CORE_MODE_AUTO;
    } else if (format_idx == 1) {
        mode = CORE_MODE_BASE64;
    } else if (format_idx == 2) {
        mode = CORE_MODE_HEX;
    } else if (format_idx == 3) {
        mode = CORE_MODE_BINARY;
    } else if (format_idx == 4) {
        mode = CORE_MODE_MORSE;
    } else {
        // Plugin selected
        PluginManager *pm = core_get_plugin_manager();
        if (pm) {
            GList *plugins = plugin_manager_list(pm);
            int idx = format_idx - 5;
            Plugin *p = g_list_nth_data(plugins, idx);
            if (p && p->decode) {
                char *output = p->decode(input);
                if (output) {
                    GtkTextBuffer *buf = gtk_text_view_get_buffer(ctx->output);
                    gtk_text_buffer_set_text(buf, output, -1);
                    gtk_label_set_text(ctx->format, p->name);
                    gtk_statusbar_push(ctx->status, 0, g_strdup_printf("Decoded using plugin: %s", p->name));
                    g_free(output);
                    g_list_free(plugins);
                    return;
                }
            }
            g_list_free(plugins);
        }
        gtk_statusbar_push(ctx->status, 0, "Plugin decode failed");
        return;
    }

    DecodeResult *res = core_decode_with_mode(input, mode);
    if (res && res->error_code == ERROR_OK) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(ctx->output);
        gtk_text_buffer_set_text(buf, res->output, -1);
        gtk_label_set_text(ctx->format, res->format);
        char msg[256];
        snprintf(msg, sizeof(msg), "Decoded in %.2f ms", res->processing_time_ms);
        gtk_statusbar_push(ctx->status, 0, msg);
    } else {
        gtk_statusbar_push(ctx->status, 0, "Failed to decode");
    }
    decode_result_free(res);
}

// Encode callback
static void on_encode_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    EncodeContext *ctx = (EncodeContext*)data;
    const char *input = gtk_entry_get_text(ctx->entry);
    if (strlen(input) == 0) {
        gtk_statusbar_push(ctx->status, 0, "Please enter text");
        return;
    }

    int format_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->combo));
    if (format_idx < 4) {
        CoreMode mode;
        const char *format_name;
        switch (format_idx) {
            case 0: mode = CORE_MODE_BASE64; format_name = "Base64"; break;
            case 1: mode = CORE_MODE_HEX; format_name = "Hex"; break;
            case 2: mode = CORE_MODE_BINARY; format_name = "Binary"; break;
            case 3: mode = CORE_MODE_MORSE; format_name = "Morse"; break;
            default: mode = CORE_MODE_BASE64; format_name = "Base64";
        }
        EncodeResult *res = core_encode(input, mode);
        if (res && res->error_code == ERROR_OK) {
            GtkTextBuffer *buf = gtk_text_view_get_buffer(ctx->output);
            gtk_text_buffer_set_text(buf, res->output, -1);
            gtk_statusbar_push(ctx->status, 0, g_strdup_printf("Encoded to %s", format_name));
        } else {
            gtk_statusbar_push(ctx->status, 0, "Failed to encode");
        }
        encode_result_free(res);
        return;
    }

    // Plugin selected
    PluginManager *pm = core_get_plugin_manager();
    if (pm) {
        GList *plugins = plugin_manager_list(pm);
        int idx = format_idx - 4;
        Plugin *p = g_list_nth_data(plugins, idx);
        if (p && p->encode) {
            char *output = p->encode(input);
            if (output) {
                GtkTextBuffer *buf = gtk_text_view_get_buffer(ctx->output);
                gtk_text_buffer_set_text(buf, output, -1);
                gtk_statusbar_push(ctx->status, 0, g_strdup_printf("Encoded using plugin: %s", p->name));
                g_free(output);
                g_list_free(plugins);
                return;
            }
        }
        g_list_free(plugins);
    }
    gtk_statusbar_push(ctx->status, 0, "Plugin encode failed");
}

// Theme toggle
static void on_theme_toggle(GtkButton *btn, gpointer data) {
    (void)btn;
    GtkWidget *window = GTK_WIDGET(data);
    theme_manager_toggle(window);
}

// Window creation
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), APP_NAME);
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Header bar with theme button
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), APP_NAME);
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    GtkWidget *theme_btn = gtk_button_new_with_label("🌓");
    g_signal_connect(theme_btn, "clicked", G_CALLBACK(on_theme_toggle), window);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), theme_btn);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    // ---------- Decode tab ----------
    GtkWidget *decode_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(decode_vbox), 10);

    GtkWidget *input_label = gtk_label_new("📝 Enter encoded text:");
    gtk_widget_set_halign(input_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(decode_vbox), input_label, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Morse, Base64, Hex, Binary...");
    gtk_box_pack_start(GTK_BOX(decode_vbox), entry, FALSE, FALSE, 0);

    // Format selection row
    GtkWidget *format_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *format_label = gtk_label_new("Format:");
    GtkWidget *format_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Auto Detect");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Base64");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Hex");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Binary");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Morse");

    // Add plugins
    PluginManager *pm = core_get_plugin_manager();
    if (pm) {
        GList *plugins = plugin_manager_list(pm);
        for (GList *iter = plugins; iter; iter = iter->next) {
            Plugin *p = (Plugin*)iter->data;
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), p->name);
        }
        g_list_free(plugins);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(format_combo), 0);
    gtk_box_pack_start(GTK_BOX(format_row), format_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(format_row), format_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(decode_vbox), format_row, FALSE, FALSE, 0);

    GtkWidget *decode_btn = gtk_button_new_with_label("🔓 Decode");
    gtk_box_pack_start(GTK_BOX(decode_vbox), decode_btn, FALSE, FALSE, 5);

    GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *detected_title = gtk_label_new("Detected format:");
    GtkWidget *detected_format = gtk_label_new("Unknown");
    gtk_box_pack_start(GTK_BOX(info_box), detected_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(info_box), detected_format, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(decode_vbox), info_box, FALSE, FALSE, 0);

    GtkWidget *output_label = gtk_label_new("📄 Result:");
    gtk_widget_set_halign(output_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(decode_vbox), output_label, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 250);
    GtkWidget *output = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), output);
    gtk_box_pack_start(GTK_BOX(decode_vbox), scrolled, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), decode_vbox, gtk_label_new("🔓 Decode"));

    // ---------- Encode tab ----------
    GtkWidget *encode_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(encode_vbox), 10);

    GtkWidget *encode_input_label = gtk_label_new("📝 Enter text:");
    gtk_widget_set_halign(encode_input_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(encode_vbox), encode_input_label, FALSE, FALSE, 0);

    GtkWidget *encode_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(encode_entry), "Text to encode...");
    gtk_box_pack_start(GTK_BOX(encode_vbox), encode_entry, FALSE, FALSE, 0);

    GtkWidget *format_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *format_choice_label = gtk_label_new("Format:");
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Base64");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Hex");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Binary");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Morse");

    if (pm) {
        GList *plugins = plugin_manager_list(pm);
        for (GList *iter = plugins; iter; iter = iter->next) {
            Plugin *p = (Plugin*)iter->data;
            if (p->encode) {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), p->name);
            }
        }
        g_list_free(plugins);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_box_pack_start(GTK_BOX(format_box), format_choice_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(format_box), combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(encode_vbox), format_box, FALSE, FALSE, 0);

    GtkWidget *encode_btn = gtk_button_new_with_label("🔒 Encode");
    gtk_box_pack_start(GTK_BOX(encode_vbox), encode_btn, FALSE, FALSE, 5);

    GtkWidget *encode_output_label = gtk_label_new("📄 Result:");
    gtk_widget_set_halign(encode_output_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(encode_vbox), encode_output_label, FALSE, FALSE, 0);

    GtkWidget *encode_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(encode_scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(encode_scrolled), 250);
    GtkWidget *encode_output = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(encode_output), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(encode_output), FALSE);
    gtk_container_add(GTK_CONTAINER(encode_scrolled), encode_output);
    gtk_box_pack_start(GTK_BOX(encode_vbox), encode_scrolled, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), encode_vbox, gtk_label_new("🔒 Encode"));

    // Status bar
    GtkWidget *statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    // Connect signals
    DecodeContext *decode_ctx = g_new0(DecodeContext, 1);
    decode_ctx->entry = GTK_ENTRY(entry);
    decode_ctx->output = GTK_TEXT_VIEW(output);
    decode_ctx->format = GTK_LABEL(detected_format);
    decode_ctx->status = GTK_STATUSBAR(statusbar);
    decode_ctx->format_combo = GTK_COMBO_BOX_TEXT(format_combo);
    g_signal_connect(decode_btn, "clicked", G_CALLBACK(on_decode_clicked), decode_ctx);

    EncodeContext *encode_ctx = g_new0(EncodeContext, 1);
    encode_ctx->entry = GTK_ENTRY(encode_entry);
    encode_ctx->output = GTK_TEXT_VIEW(encode_output);
    encode_ctx->combo = GTK_COMBO_BOX_TEXT(combo);
    encode_ctx->status = GTK_STATUSBAR(statusbar);
    g_signal_connect(encode_btn, "clicked", G_CALLBACK(on_encode_clicked), encode_ctx);

    // Set window icon (optional)
    GdkPixbuf *icon = gdk_pixbuf_new_from_file("ui/resources/icons/app-icon.png", NULL);
    if (icon) {
        gtk_window_set_icon(GTK_WINDOW(window), icon);
        g_object_unref(icon);
    }

    // Apply theme
    theme_manager_apply(window, theme_manager_get_current());

    gtk_widget_show_all(window);
}

// Main function
int main(int argc, char **argv) {
    install_crash_handler();
    log_init(LOG_LEVEL_INFO, NULL, TRUE);
    core_init();
    theme_manager_init();

    GtkApplication *app = gtk_application_new("com.autodecoder.pro", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    (void)status;
    g_object_unref(app);

    core_cleanup();
    log_close();
    return 0;
}