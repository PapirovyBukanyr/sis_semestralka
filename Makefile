
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

# collect receiver sources (core + modules)
RECEIVER_SRCS := $(wildcard receiver/*.c) $(wildcard receiver/module1/*.c) $(wildcard receiver/module2/*.c) $(wildcard receiver/module3/*.c) $(wildcard receiver/module4/*.c)

all: $(BINDIR)/net_logger $(BINDIR)/analyzer

$(BINDIR)/net_logger: sender/net_logger.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BINDIR)/analyzer: $(RECEIVER_SRCS)
	$(MKDIR_P)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)

.PHONY: analyzer-sdl
ifeq ($(strip $(SDL_LIBS)),)
analyzer-sdl:
	$(error SDL2 not found via pkg-config; please install SDL2 dev packages or set LDFLAGS/CFLAGS accordingly.)
else
analyzer-sdl: $(RECEIVER_SRCS)
	$(MKDIR_P)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)
endif

clean:
	rm -rf $(BINDIR)

.PHONY: all clean run-windows
