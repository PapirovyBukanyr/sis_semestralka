# Compiler and directories
CC = gcc
SRCDIR = receiver
BINDIR = bin

# Note: do not force `SHELL := powershell` here. Let make use the
# platform-default shell (cmd.exe on Windows) to avoid cross-shell
# quoting/parsing issues. Recipes below choose Windows- or POSIX-friendly
# commands via make conditionals.

# SDL2 detection (optional)
SDL_CFLAGS =
SDL_LIBS =

# Windows-specific settings
ifeq ($(OS),Windows_NT)
MKDIR_P = powershell -Command "if (!(Test-Path '$(BINDIR)')) { New-Item -ItemType Directory -Path '$(BINDIR)' }"
CFLAGS = -O2 -Wall
LDFLAGS = -lws2_32
else
MKDIR_P = mkdir -p $(BINDIR)
CFLAGS = -O2 -Wall -pthread
LDFLAGS =
endif

# Collect receiver sources
RECEIVER_SRCS := $(wildcard receiver/*.c) \
                 $(wildcard receiver/module1/*.c) \
                 $(wildcard receiver/module2/*.c) \
                 $(wildcard receiver/module3/*.c) \
                 $(wildcard receiver/module4/*.c)

RECEIVER_SRCS := $(filter-out receiver/module2/nn.c,$(RECEIVER_SRCS))

.PHONY: all clean run-windows analyzer-sdl

all:  $(BINDIR)/net_logger $(BINDIR)/analyzer

$(BINDIR)/net_logger: sender/net_logger.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BINDIR)/analyzer: $(RECEIVER_SRCS)
	$(MKDIR_P)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)

# Clean build artifacts
clean:
	Remove-Item -Recurse -Force $(BINDIR)