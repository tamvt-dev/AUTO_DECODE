CC = gcc
CFLAGS = -Wall -O2 -fPIC -shared
LDFLAGS = -shared

# Các plugin
PLUGINS = base64_plugin.dll rot13_plugin.dll url_plugin.dll atbash_plugin.dll caesar_plugin.dll

all: $(PLUGINS)

%.dll: %.c
	$(CC) $(CFLAGS) -o $@ $^ -I../include -I../ui `pkg-config --cflags glib-2.0 gmodule-2.0`

clean:
	rm -f $(PLUGINS)

install:
	mkdir -p ../build/bin/plugins
	cp $(PLUGINS) ../build/bin/plugins/

.PHONY: all clean install