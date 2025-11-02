CC = gcc
SRCDIR = receiver
BINDIR = bin

ifeq ($(OS),Windows_NT)
	# Run directory creation via cmd to avoid PowerShell / mkdir -p incompatibilities
	MKDIR_P = cmd /C "if not exist $(BINDIR) mkdir $(BINDIR)"
	CFLAGS = -O2 -Wall
	LDFLAGS = -lws2_32
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

$(BINDIR)/analyzer: receiver/main.c receiver/common.c receiver/module1/preproc.c receiver/module1/convertor.c receiver/module2/nn.c receiver/module3/represent.c receiver/module4/ui.c
	$(MKDIR_P)
	$(CC) $(CFLAGS) -o $@ $^ -lm $(LDFLAGS)

clean:
	 rm -rf $(BINDIR)

.PHONY: all clean
