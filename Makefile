# Auto Decoder Pro - Production Makefile
VERSION := 2.0.0
PREFIX ?= /usr/local

CC := gcc
PKG_CONFIG := pkg-config

# Build type: release, debug, profile
BUILD ?= release

# Flags
CFLAGS_BASE := -Wall -Wextra -pthread -DVERSION=\"$(VERSION)\" -D_GNU_SOURCE
CFLAGS_OPT := -O3 -march=native -flto -fomit-frame-pointer -fstack-protector-strong
CFLAGS_DEBUG := -g -O0 -DDEBUG -fsanitize=address,undefined

ifeq ($(BUILD),debug)
    CFLAGS := $(CFLAGS_BASE) $(CFLAGS_DEBUG)
    LDFLAGS := -fsanitize=address,undefined
    SUFFIX := -debug
else
    CFLAGS := $(CFLAGS_BASE) $(CFLAGS_OPT) -DNDEBUG
    LDFLAGS := -flto -Wl,-O2
    SUFFIX :=
endif

# Dependencies
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0 glib-2.0 2>/dev/null)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0 glib-2.0 gmodule-2.0 2>/dev/null)

CFLAGS += $(GTK_CFLAGS)
LDFLAGS += $(GTK_LIBS) -lm

# Directories
SRCDIR := src
INCDIR := include
UIDIR := ui
OBJDIR := build/obj
BINDIR := build/bin

# Source files
CORE_SRC := $(SRCDIR)/core.c $(SRCDIR)/decoder.c $(SRCDIR)/encoder.c \
            $(SRCDIR)/plugin.c $(SRCDIR)/lru_cache.c $(SRCDIR)/logging.c \
            $(SRCDIR)/errors.c $(SRCDIR)/crash_handler.c

UI_SRC := $(UIDIR)/gui.c
GUI_SRC := $(SRCDIR)/main.c
CLI_SRC := $(SRCDIR)/cli.c

# Objects
CORE_OBJ := $(CORE_SRC:%.c=$(OBJDIR)/%.o)
UI_OBJ := $(UI_SRC:%.c=$(OBJDIR)/%.o)
GUI_OBJ := $(GUI_SRC:%.c=$(OBJDIR)/%.o) $(CORE_OBJ) $(UI_OBJ)
CLI_OBJ := $(CLI_SRC:%.c=$(OBJDIR)/%.o) $(CORE_OBJ)

# Targets
.PHONY: all clean install uninstall gui cli help

all: gui cli

gui: $(BINDIR)/auto_decoder_pro$(SUFFIX)

cli: $(BINDIR)/auto_decoder$(SUFFIX)

$(BINDIR)/auto_decoder_pro$(SUFFIX): $(GUI_OBJ)
	@mkdir -p $(BINDIR)
	@echo "Building GUI ($(BUILD) mode)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ GUI built: $@"

$(BINDIR)/auto_decoder$(SUFFIX): $(CLI_OBJ)
	@mkdir -p $(BINDIR)
	@echo "Building CLI ($(BUILD) mode)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ CLI built: $@"

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(UIDIR) -c $< -o $@

clean:
	@echo "Cleaning..."
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "✅ Clean complete"

install: all
	@echo "Installing to $(PREFIX)..."
	install -Dm755 $(BINDIR)/auto_decoder_pro$(SUFFIX) $(PREFIX)/bin/auto_decoder_pro
	install -Dm755 $(BINDIR)/auto_decoder$(SUFFIX) $(PREFIX)/bin/auto_decoder
	@echo "✅ Installation complete"

uninstall:
	@echo "Uninstalling..."
	rm -f $(PREFIX)/bin/auto_decoder_pro
	rm -f $(PREFIX)/bin/auto_decoder
	@echo "✅ Uninstall complete"

help:
	@echo "Auto Decoder Pro Build System v$(VERSION)"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build GUI and CLI (default)"
	@echo "  gui        - Build GUI only"
	@echo "  cli        - Build CLI only"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to system"
	@echo "  uninstall  - Remove from system"