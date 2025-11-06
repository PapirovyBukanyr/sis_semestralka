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

# Exclude alternative/duplicate neuron implementation to avoid multiple-definition
# link errors. The canonical implementation lives in receiver/module2/neuron_layer.c
RECEIVER_SRCS := $(filter-out receiver/module2/neuron.c,$(RECEIVER_SRCS))

# Targets
.PHONY: all clean run-windows analyzer-sdl check-compiler

all: check-compiler $(BINDIR)/net_logger $(BINDIR)/analyzer

# Check for a C compiler in a cross-platform way. On Windows use PowerShell
# (or cmd) invocation; on POSIX use sh. Avoid sending `#`-prefixed lines into
# shells that don't treat `#` as comment (cmd.exe).
## Check for a C compiler in a cross-platform way. Use make's conditional
## to choose the appropriate recipe so the shell used to execute recipes
## doesn't need to parse POSIX `if` syntax.
ifeq ($(OS),Windows_NT)
check-compiler:
	@powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\\check-compiler.ps1"
else
check-compiler:
	@sh -c 'command -v gcc >/dev/null 2>&1 || command -v cc >/dev/null 2>&1 || command -v clang >/dev/null 2>&1 || { echo "No C compiler found in PATH. Install GCC or clang."; exit 1; }'
endif

$(BINDIR)/net_logger: sender/net_logger.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BINDIR)/analyzer: $(RECEIVER_SRCS)
	$(MKDIR_P)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)

# SDL analyzer build
# ifeq ($(strip $(SDL_LIBS)),)
# analyzer-sdl:
#	@echo "SDL2 not found via pkg-config; please install SDL2 dev packages or set LDFLAGS/CFLAGS accordingly."
# else
# analyzer-sdl: $(RECEIVER_SRCS)
#	$(MKDIR_P)
#	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $^ -lm $(SDL_LIBS) $(LDFLAGS)
# endif

# Run analyzer script on Windows
run-windows:
	powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\\run-analyzer.ps1"

# Clean build artifacts
clean:
	Remove-Item -Recurse -Force $(BINDIR)