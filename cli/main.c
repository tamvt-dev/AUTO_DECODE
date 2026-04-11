#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "../core/include/core.h"
#include "../core/include/pipeline.h"
#include "../core/include/buffer.h"
#include "../core/include/version.h"
#include "../core/include/logging.h"
#include "../core/include/score.h"
#include "../core/include/decoder.h"

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

// ANSI Color Codes
#define CLR_RST  "\x1b[0m"
#define CLR_GRN  "\x1b[32m"
#define CLR_RED  "\x1b[31m"
#define CLR_YEL  "\x1b[33m"
#define CLR_BLU  "\x1b[34m"
#define CLR_MAG  "\x1b[35m"
#define CLR_CYN  "\x1b[36m"
#define CLR_WHT  "\x1b[37m"
#define CLR_BOLD "\x1b[1m"

typedef struct {
    gboolean help;
    gboolean pipeline;
    gboolean fast;
    gboolean deep;
    gboolean raw;
    gboolean json;
    gboolean verbose;
    gboolean trace;
    gboolean version;
    gint max_depth;
    gint top_n;
} CliOptions;

static gchar* read_stdin_input(void) {
    GString *buffer = g_string_new(NULL);
    char chunk[4096];

    while (fgets(chunk, sizeof(chunk), stdin)) {
        g_string_append(buffer, chunk);
    }

    if (buffer->len == 0) {
        g_string_free(buffer, TRUE);
        return NULL;
    }

    return g_strstrip(g_string_free(buffer, FALSE));
}

static gchar* read_input_argument(const char *arg) {
    gchar *contents = NULL;
    gsize length = 0;

    if (g_file_test(arg, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) &&
        g_file_get_contents(arg, &contents, &length, NULL)) {
        return g_strstrip(contents);
    }

    return g_strdup(arg);
}

static gchar* json_escape(const char *input) {
    if (!input) {
        return g_strdup("");
    }

    GString *out = g_string_new(NULL);
    for (const unsigned char *p = (const unsigned char*)input; *p; ++p) {
        switch (*p) {
            case '\\': g_string_append(out, "\\\\"); break;
            case '"':  g_string_append(out, "\\\""); break;
            case '\n': g_string_append(out, "\\n"); break;
            case '\r': g_string_append(out, "\\r"); break;
            case '\t': g_string_append(out, "\\t"); break;
            default:
                if (*p < 0x20) {
                    g_string_append_printf(out, "\\u%04x", *p);
                } else {
                    g_string_append_c(out, (char)*p);
                }
                break;
        }
    }
    return g_string_free(out, FALSE);
}

static gchar* join_steps(GList *steps) {
    GString *joined = g_string_new(NULL);
    for (GList *iter = steps; iter; iter = iter->next) {
        if (joined->len > 0) {
            g_string_append(joined, " -> ");
        }
        g_string_append(joined, (const char*)iter->data);
    }
    return g_string_free(joined, FALSE);
}

static gchar* candidate_output(const Candidate *candidate) {
    return g_strndup((const char*)candidate->buf.data, candidate->buf.len);
}

static double clamp01(double value) {
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

static double text_printable_ratio(const char *text) {
    if (!text || !*text) {
        return 0.0;
    }

    size_t len = strlen(text);
    size_t printable = 0;
    for (size_t i = 0; i < len; i++) {
        if (g_ascii_isprint((guchar)text[i])) {
            printable++;
        }
    }
    return (double)printable / (double)len;
}

static gboolean text_has_alpha(const char *text) {
    if (!text) {
        return FALSE;
    }
    for (const unsigned char *p = (const unsigned char*)text; *p; ++p) {
        if (g_ascii_isalpha(*p)) {
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean text_is_structured_artifact(const char *text) {
    if (!text) {
        return FALSE;
    }
    size_t len = strlen(text);
    return is_hex(text, len) || is_binary(text, len) || is_morse(text, len) || is_base64(text, len);
}

static double confidence_from_text(const char *text, double base_score) {
    if (!text || !*text) {
        return 0.0;
    }

    double confidence = clamp01(base_score);
    double printable = text_printable_ratio(text);
    gboolean has_alpha = text_has_alpha(text);
    gboolean structured = text_is_structured_artifact(text);

    if (printable < 0.85) confidence -= 0.35;
    if (printable < 0.60) confidence -= 0.20;
    if (structured) confidence -= 0.20;
    if (!has_alpha) confidence -= 0.12;
    if (printable > 0.95 && has_alpha && !structured) confidence += 0.10;

    return clamp01(confidence);
}

static double decode_confidence(const DecodeResult *result) {
    if (!result || result->error_code != ERROR_OK || !result->output) {
        return 0.0;
    }

    size_t len = strlen(result->output);
    if (len == 0) {
        return 0.0;
    }

    return confidence_from_text(result->output, score_readability((const unsigned char*)result->output, len) / 2.0);
}

static double pipeline_confidence(const Candidate *candidate) {
    if (!candidate) {
        return 0.0;
    }

    gchar *output = candidate_output(candidate);
    double confidence = confidence_from_text(output, candidate->score / 2.0);
    g_free(output);
    return confidence;
}

static const char* confidence_label_colored(double confidence) {
    if (confidence >= 0.75) return CLR_GRN "high" CLR_RST;
    if (confidence >= 0.45) return CLR_YEL "medium" CLR_RST;
    return CLR_RED "low" CLR_RST;
}

static const char* confidence_label(double confidence) {
    if (confidence >= 0.75) return "high";
    if (confidence >= 0.45) return "medium";
    return "low";
}

static void print_usage(void) {
    printf(CLR_CYN "   _  _                       " CLR_GRN " ___                          _      " CLR_RST "\n");
    printf(CLR_CYN "  | || | _  _  _ __  ___  _ _ " CLR_GRN "|   \\  ___  __  ___  __| | ___ " CLR_RST "\n");
    printf(CLR_CYN "  | __ || || || '_ \\/ -_)| '_|" CLR_GRN "| |) |/ -_)/ _|/ _ \\/ _` |/ -_)" CLR_RST "\n");
    printf(CLR_CYN "  |_||_| \\_, || .__/\\___||_|  " CLR_GRN "|___/ \\___|\\__|\\___/\\__,_|\\___|" CLR_RST "\n");
    printf(CLR_CYN "         |__/ |_|             " CLR_RST "\n\n");
    
    printf(CLR_GRN CLR_BOLD "HyperDecode v%s" CLR_RST " - AI-powered multi-layer decoding engine\n\n", APP_VERSION);
    printf(CLR_CYN "Usage:" CLR_RST "\n");
    printf("  hyperdecode [input]\n");
    printf("  hyperdecode [input] [options]\n");
    printf("  echo \"SGVsbG8=\" | hyperdecode --pipeline\n\n");
    printf(CLR_CYN "Options:" CLR_RST "\n");
    printf("  " CLR_YEL "--pipeline" CLR_RST "        run full layered decoding\n");
    printf("  " CLR_YEL "--fast" CLR_RST "            skip heavy search\n");
    printf("  " CLR_YEL "--deep" CLR_RST "            enable recursive decoding\n");
    printf("  " CLR_YEL "--max-depth" CLR_RST " N     limit pipeline depth\n\n");
    printf(CLR_CYN "Output:" CLR_RST "\n");
    printf("  " CLR_YEL "--raw" CLR_RST "             plain text output only\n");
    printf("  " CLR_YEL "--json" CLR_RST "            output JSON\n");
    printf("  " CLR_YEL "--top" CLR_RST " N           show top N results\n\n");
    printf(CLR_CYN "Debug:" CLR_RST "\n");
    printf("  " CLR_YEL "--verbose" CLR_RST "         show basic info\n");
    printf("  " CLR_YEL "--trace" CLR_RST "           show full pipeline steps\n\n");
    printf(CLR_CYN "Other:" CLR_RST "\n");
    printf("  " CLR_YEL "--version" CLR_RST "         show version\n");
    printf("  " CLR_YEL "--help" CLR_RST "            show this help\n\n");
    printf(CLR_CYN "Examples:" CLR_RST "\n");
    printf("  " CLR_BOLD "hyperdecode SGVsbG8=" CLR_RST "\n");
    printf("  " CLR_BOLD "hyperdecode input.txt --pipeline --trace" CLR_RST "\n");
    printf("  " CLR_BOLD "cat file.bin | hyperdecode --raw" CLR_RST "\n");
}

static void print_decode_result_plain(const DecodeResult *result, const CliOptions *options) {
    if (!result || result->error_code != ERROR_OK || !result->output) {
        fprintf(stderr, "Error: %s\n",
                (result && result->error_message) ? result->error_message : "Decode failed");
        return;
    }

    printf("%s\n", result->output);

    if (options->verbose || options->trace) {
        double confidence = decode_confidence(result);
        fprintf(stderr, "\n" CLR_CYN "[Decode]" CLR_RST "\n");
        fprintf(stderr, "Format: " CLR_MAG "%s" CLR_RST "\n", result->format ? result->format : "Unknown");
        fprintf(stderr, "Confidence: %.2f (%s)\n", confidence, confidence_label_colored(confidence));
        fprintf(stderr, "Time: %.2f ms\n", result->processing_time_ms);
        fprintf(stderr, "Input bytes: %zu\n", result->input_size);
        fprintf(stderr, "Output bytes: %zu\n", result->output_size);
        fprintf(stderr, "Depth: %d\n", result->decode_depth);
    }
}

static void print_decode_result_json(const DecodeResult *result) {
    gchar *output = json_escape(result && result->output ? result->output : "");
    gchar *format = json_escape(result && result->format ? result->format : "");
    gchar *error = json_escape(result && result->error_message ? result->error_message : "");
    double confidence = decode_confidence(result);

    printf("{\n");
    printf("  \"" CLR_YEL "success" CLR_RST "\": %s,\n", (result && result->error_code == ERROR_OK) ? CLR_GRN "true" CLR_RST : CLR_RED "false" CLR_RST);
    printf("  \"" CLR_YEL "mode" CLR_RST "\": \"" CLR_CYN "decode" CLR_RST "\",\n");
    printf("  \"" CLR_YEL "format" CLR_RST "\": \"" CLR_MAG "%s" CLR_RST "\",\n", format);
    printf("  \"" CLR_YEL "output" CLR_RST "\": \"%s\",\n", output);
    printf("  \"" CLR_YEL "confidence" CLR_RST "\": " CLR_GRN "%.3f" CLR_RST ",\n", confidence);
    printf("  \"" CLR_YEL "confidence_label" CLR_RST "\": \"%s\",\n", confidence_label_colored(confidence));
    printf("  \"" CLR_YEL "processing_time_ms" CLR_RST "\": " CLR_CYN "%.3f" CLR_RST ",\n", result ? result->processing_time_ms : 0.0);
    printf("  \"" CLR_YEL "decode_depth" CLR_RST "\": %d,\n", result ? result->decode_depth : 0);
    printf("  \"" CLR_YEL "error" CLR_RST "\": \"%s\"\n", error);
    printf("}\n");

    g_free(output);
    g_free(format);
    g_free(error);
}

static void print_pipeline_result_plain(GList *candidates, gint top_n, const CliOptions *options) {
    gint index = 0;

    for (GList *iter = candidates; iter && index < top_n; iter = iter->next, ++index) {
        Candidate *candidate = (Candidate*)iter->data;
        gchar *route = join_steps(candidate->steps);
        gchar *output = candidate_output(candidate);
        double confidence = pipeline_confidence(candidate);

        if (top_n == 1) {
            printf("%s\n", output);
        } else {
            printf("[%d] %s\n", index + 1, output);
        }

        if (options->verbose || options->trace || top_n > 1) {
            fprintf(stderr, "\n" CLR_CYN "[Result %d]" CLR_RST "\n", index + 1);
            fprintf(stderr, "Score: %.3f\n", candidate->score);
            fprintf(stderr, "Confidence: %.2f (%s)\n", confidence, confidence_label_colored(confidence));
            fprintf(stderr, "Route: " CLR_MAG "%s" CLR_RST "\n", route && *route ? route : "Input");
            if (candidate->meta && *candidate->meta) {
                fprintf(stderr, "Meta: %s\n", candidate->meta);
            }
        }

        if (options->trace) {
            fprintf(stderr, "Steps:\n");
            for (GList *step = candidate->steps; step; step = step->next) {
                fprintf(stderr, "  - %s\n", (const char*)step->data);
            }
        }

        g_free(route);
        g_free(output);
    }
}

static void print_pipeline_result_json(GList *candidates, gint top_n) {
    printf("{\n");
    printf("  \"" CLR_YEL "success" CLR_RST "\": " CLR_GRN "true" CLR_RST ",\n");
    printf("  \"" CLR_YEL "mode" CLR_RST "\": \"" CLR_CYN "pipeline" CLR_RST "\",\n");
    printf("  \"" CLR_YEL "results" CLR_RST "\": [\n");

    gint index = 0;
    for (GList *iter = candidates; iter && index < top_n; iter = iter->next, ++index) {
        Candidate *candidate = (Candidate*)iter->data;
        gchar *output = candidate_output(candidate);
        gchar *route = join_steps(candidate->steps);
        gchar *output_json = json_escape(output);
        gchar *route_json = json_escape(route);
        gchar *meta_json = json_escape(candidate->meta ? candidate->meta : "");
        double confidence = pipeline_confidence(candidate);

        printf("    {\n");
        printf("      \"" CLR_YEL "rank" CLR_RST "\": %d,\n", index + 1);
        printf("      \"" CLR_YEL "output" CLR_RST "\": \"%s\",\n", output_json);
        printf("      \"" CLR_YEL "score" CLR_RST "\": " CLR_GRN "%.3f" CLR_RST ",\n", candidate->score);
        printf("      \"" CLR_YEL "confidence" CLR_RST "\": " CLR_CYN "%.3f" CLR_RST ",\n", confidence);
        printf("      \"" CLR_YEL "confidence_label" CLR_RST "\": \"%s\",\n", confidence_label_colored(confidence));
        printf("      \"" CLR_YEL "route" CLR_RST "\": \"" CLR_MAG "%s" CLR_RST "\",\n", route_json);
        printf("      \"" CLR_YEL "meta" CLR_RST "\": \"%s\"\n", meta_json);
        printf("    }%s\n", (iter->next && index + 1 < top_n) ? "," : "");

        g_free(output);
        g_free(route);
        g_free(output_json);
        g_free(route_json);
        g_free(meta_json);
    }

    printf("  ]\n");
    printf("}\n");
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // Enable ANSI support on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    if (hErr != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hErr, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hErr, dwMode);
        }
    }
#endif
    CliOptions options = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, 6, 1 };
    GError *error = NULL;
    gchar *stdin_input = NULL;
    gchar *input = NULL;

    GOptionEntry entries[] = {
        { "help", 'h', 0, G_OPTION_ARG_NONE, &options.help, "Show this help", NULL },
        { "pipeline", 0, 0, G_OPTION_ARG_NONE, &options.pipeline, "Run full layered decoding", NULL },
        { "fast", 0, 0, G_OPTION_ARG_NONE, &options.fast, "Skip heavy search", NULL },
        { "deep", 0, 0, G_OPTION_ARG_NONE, &options.deep, "Enable recursive decoding", NULL },
        { "raw", 0, 0, G_OPTION_ARG_NONE, &options.raw, "Plain text output only", NULL },
        { "json", 0, 0, G_OPTION_ARG_NONE, &options.json, "Output JSON", NULL },
        { "js", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &options.json, "Output JSON", NULL },
        { "top", 0, 0, G_OPTION_ARG_INT, &options.top_n, "Show top N results", "N" },
        { "verbose", 0, 0, G_OPTION_ARG_NONE, &options.verbose, "Show basic info", NULL },
        { "trace", 0, 0, G_OPTION_ARG_NONE, &options.trace, "Show full pipeline steps", NULL },
        { "max-depth", 0, 0, G_OPTION_ARG_INT, &options.max_depth, "Limit pipeline depth", "N" },
        { "version", 0, 0, G_OPTION_ARG_NONE, &options.version, "Show version", NULL },
        { NULL }
    };

    GOptionContext *context = g_option_context_new("[input]");
    g_option_context_set_summary(context, "AI-powered multi-layer decoding engine.");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_set_help_enabled(context, FALSE);

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        fprintf(stderr, "Error: %s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);
        return 1;
    }

    if (options.version) {
        printf("%s %s\n", APP_NAME, APP_VERSION);
        g_option_context_free(context);
        return 0;
    }

    if (options.help) {
        print_usage();
        g_option_context_free(context);
        return 0;
    }

    if (argc > 1) {
        if (argc == 2) {
            // Standard single argument / file path logic
            input = read_input_argument(argv[1]);
        } else {
            // Join all remaining arguments with a space
            GString *joined = g_string_new(NULL);
            for (int i = 1; i < argc; i++) {
                if (i > 1) g_string_append_c(joined, ' ');
                g_string_append(joined, argv[i]);
            }
            input = g_string_free(joined, FALSE);
        }
    } else {
        stdin_input = read_stdin_input();
        input = stdin_input ? g_strdup(stdin_input) : NULL;
    }

    if (!input || !*g_strstrip(input)) {
        print_usage();
        g_free(stdin_input);
        g_free(input);
        g_option_context_free(context);
        return 1;
    }

    if (options.max_depth < 1) {
        options.max_depth = 1;
    }

    if (options.top_n < 1) {
        options.top_n = 1;
    }

    if (options.raw) {
        options.json = FALSE;
    }

    log_init(LOG_LEVEL_WARN, NULL, TRUE);
    core_init();

    int exit_code = 0;

    if (options.pipeline) {
        int beam_width = options.fast ? 3 : MAX(15, options.top_n);
        int max_depth = options.fast ? MIN(options.max_depth, 2) : options.max_depth;

        Buffer in = buffer_new((const unsigned char*)input, strlen(input));
        GList *candidates = pipeline_smart_search(in, max_depth, beam_width);
        buffer_free(&in);

        if (!candidates) {
            if (options.json) {
                printf("{\n");
                printf("  \"success\": false,\n");
                printf("  \"mode\": \"pipeline\",\n");
                printf("  \"error\": \"No pipeline result\"\n");
                printf("}\n");
            } else {
                fprintf(stderr, "Error: No pipeline result\n");
            }
            exit_code = 1;
        } else {
            if (options.json) {
                print_pipeline_result_json(candidates, options.top_n);
            } else {
                print_pipeline_result_plain(candidates, options.top_n, &options);
            }
            candidate_list_free(candidates);
        }
    } else {
        DecodeResult *result = options.deep
            ? core_decode_recursive(input, options.max_depth)
            : core_decode(input);

        if (options.json) {
            print_decode_result_json(result);
        } else {
            print_decode_result_plain(result, &options);
        }

        if (!result || result->error_code != ERROR_OK) {
            exit_code = 1;
        }

        decode_result_free(result);
    }

    core_cleanup();
    log_close();

    g_free(stdin_input);
    g_free(input);
    g_option_context_free(context);
    return exit_code;
}
