# Compiler and directories
CC = gcc
SRCDIR = receiver
BINDIR = bin

# Optional OpenAI support: set `USE_OPENAI=1` when calling make to link libcurl
USE_OPENAI ?= 0

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
# keep winsock library on Windows; curl linking is opt-in via USE_OPENAI
LDFLAGS = -lws2_32
else
MKDIR_P = mkdir -p $(BINDIR)
CFLAGS = -O2 -Wall -pthread
# Link libcurl on non-Windows platforms for OpenAI client (opt-in)
LDFLAGS =
endif

ifneq ($(USE_OPENAI),0)
# Add libcurl when OpenAI support is requested; also provide a macro
LDFLAGS += -lcurl
CFLAGS += -DOPENAI_ENABLED
endif

# Collect receiver sources
RECEIVER_SRCS := $(wildcard receiver/*.c) \
				 $(wildcard receiver/module1/*.c) \
				 $(wildcard receiver/module2/*.c) \
				 $(wildcard receiver/module3/*.c) \
				 $(wildcard receiver/module4/*.c)

# Exclude files that should not be built by default
RECEIVER_SRCS := $(filter-out receiver/module2/nn.c,$(RECEIVER_SRCS))

# OpenAI client is optional. If OpenAI support is disabled, remove the
# client implementation from the build to avoid requiring libcurl headers.
ifneq ($(USE_OPENAI),0)
RECEIVER_SRCS := $(filter-out ,$(RECEIVER_SRCS))
else
RECEIVER_SRCS := $(filter-out receiver/module3/openai_client.c,$(RECEIVER_SRCS))
endif

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