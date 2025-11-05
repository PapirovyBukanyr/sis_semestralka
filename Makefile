# Compiler and directories
CC = gcc
SRCDIR = receiver
BINDIR = bin

# Use PowerShell as the shell
SHELL := powershell
.SHELLFLAGS := -NoProfile -ExecutionPolicy Bypass -Command

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

# Targets
.PHONY: all clean run-windows analyzer-sdl check-compiler

all: check-compiler $(BINDIR)/net_logger $(BINDIR)/analyzer

check-compiler:
	# PowerShell: ensure at least one C compiler (gcc, cc, or cl) is available
	if (-not (Get-Command gcc -ErrorAction SilentlyContinue) -and -not (Get-Command cc -ErrorAction SilentlyContinue) -and -not (Get-Command cl -ErrorAction SilentlyContinue)) { Write-Error "No C compiler found in PATH. Install GCC (MinGW/MSYS) or MSVC and ensure it's on PATH."; exit 1 } else { Write-Output "C compiler found." }

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