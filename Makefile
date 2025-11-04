CC = gcc
SRCDIR = receiver
BINDIR = bin

# Try to detect SDL2 via pkg-config; will be empty if not available.
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null || echo)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null || echo)

ifeq ($(OS),Windows_NT)
	# Run directory creation via cmd to avoid PowerShell / mkdir -p incompatibilities
	MKDIR_P = cmd /C "if not exist $(BINDIR) mkdir $(BINDIR)"
	CFLAGS = -O2 -Wall
	LDFLAGS = -lws2_32

	# Convenience: run analyzer on Windows via PowerShell helper that copies SDL2.dll when available
	.PHONY: run-windows
	run-windows:
		powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\\run-analyzer.ps1"
else
	MKDIR_P = mkdir -p $(BINDIR)
	CFLAGS = -O2 -Wall -pthread
	LDFLAGS =
endif

ifeq ($(OS),Windows_NT)
all: $(BINDIR)/net_logger $(BINDIR)/analyzer
else
all: $(BINDIR)/net_logger $(BINDIR)/analyzer
endif

$(BINDIR)/net_logger: sender/net_logger.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BINDIR)/analyzer: receiver/main.c receiver/common.c receiver/module1/preproc.c receiver/module1/convertor.c receiver/module2/nn.c receiver/module2/neuron.c receiver/module2/neuron_layer.c receiver/module3/represent.c receiver/module4/ui.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)

.PHONY: analyzer-sdl
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)

analyzer-sdl: receiver/main.c receiver/common.c receiver/module1/preproc.c receiver/module1/convertor.c receiver/module2/nn.c receiver/module2/neuron.c receiver/module2/neuron_layer.c receiver/module3/represent.c receiver/module4/ui.c
	$(MKDIR_P)
	@if [ -z "$(SDL_LIBS)" ]; then echo "SDL2 not found via pkg-config; please install SDL2 dev packages or set LDFLAGS/CFLAGS accordingly."; exit 1; fi
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)

clean:
	 rm -rf $(BINDIR)

.PHONY: all clean
