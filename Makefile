# Bikini - Small, minimal INI library and query tool.
# Copyright (C) 2026 Fabrika Charvat s.r.o. All rights reserved.
# Developed by Partoska Laboratory team, <https://lab.partoska.com>
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# You can contact the author(s) via email at ask <at> partoska.com.
#


# Plain make build of the bikini tool and libbikini, for systems without
# CMake (POSIX make with a C99 compiler). CMake stays the reference build:
# keep the sources and flags below in step with it.

# Project configuration.
PROJECT = bikini
LIBRARY = bikini
VERSION = 2.0.0
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

# Build configuration.
BUILD_TYPE ?= release
SANITIZERS ?= 0
VERBOSE_DEBUG ?= 0

# Directories.
SRCDIR = src
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
TESTDIR = test
TESTBINDIR = $(BUILDDIR)/test

# Compiler configuration.
CC ?= cc
AR ?= ar
ARFLAGS = rcs

# Sources.
LIB_SRCS = $(SRCDIR)/lib/src/plat.c \
           $(SRCDIR)/lib/src/bikini.c
CLI_SRCS = $(SRCDIR)/cli/src/plat.c \
           $(SRCDIR)/cli/src/main.c
LIB_HDRS = $(SRCDIR)/lib/inc/bktypes.h \
           $(SRCDIR)/lib/inc/bikini.h

LIB_OBJS = $(LIB_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
CLI_OBJS = $(CLI_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(LIB_OBJS:.o=.d) $(CLI_OBJS:.o=.d)

# Compiler flags.
CFLAGS_COMMON = -std=c99 -fno-strict-aliasing -pedantic \
                -Wall -Werror -Wextra -Werror=return-type -Werror=array-bounds
CFLAGS_DEBUG = -O0 -g3 -D_DEBUG=1
CFLAGS_RELEASE = -Os -g0 -DNDEBUG

ifeq ($(BUILD_TYPE),debug)
    CFLAGS_BUILD = $(CFLAGS_DEBUG)
else
    CFLAGS_BUILD = $(CFLAGS_RELEASE)
endif

ifeq ($(VERBOSE_DEBUG),1)
    CFLAGS_BUILD += -DDEBUG_SLOW=1
endif

ifeq ($(SANITIZERS),1)
    SANITIZER_FLAGS = -fno-omit-frame-pointer \
                      -fsanitize=undefined \
                      -fsanitize=address \
                      -fsanitize=float-cast-overflow \
                      -fsanitize-address-use-after-scope \
                      -fno-sanitize-recover
    CFLAGS_COMMON += $(SANITIZER_FLAGS)
    LDFLAGS += $(SANITIZER_FLAGS)
endif

DEPFLAGS = -MMD -MP
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_BUILD) -I$(SRCDIR)/lib/inc

# Target artifacts.
LIBTARGET = $(BUILDDIR)/lib$(LIBRARY).a
TARGET = $(BUILDDIR)/$(PROJECT)
TEST_BINS = $(TESTBINDIR)/test_bikini

.PHONY: all clean install install-strip uninstall help debug release tests test

# Default target.
all: $(TARGET)

# Executable, linked against the static library.
$(TARGET): $(CLI_OBJS) $(LIBTARGET)
	@echo "Linking $(PROJECT)..."
	$(CC) $(CLI_OBJS) $(LIBTARGET) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

$(LIBTARGET): $(LIB_OBJS)
	@echo "Archiving lib$(LIBRARY)..."
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TESTBINDIR)/%: $(TESTDIR)/%.c $(LIBTARGET)
	@mkdir -p $(TESTBINDIR)
	@echo "Building test $*..."
	$(CC) $(CFLAGS) $< $(LIBTARGET) -o $@ $(LDFLAGS)

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILDDIR)

install: $(TARGET)
	@echo "Installing $(PROJECT) to $(PREFIX)..."
	install -d $(BINDIR) $(LIBDIR) $(INCLUDEDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(PROJECT)
	install -m 644 $(LIBTARGET) $(LIBDIR)/lib$(LIBRARY).a
	install -m 644 $(LIB_HDRS) $(INCLUDEDIR)/

install-strip: $(TARGET)
	@$(MAKE) install
	strip $(BINDIR)/$(PROJECT)

uninstall:
	@echo "Uninstalling $(PROJECT) from $(PREFIX)..."
	rm -f $(BINDIR)/$(PROJECT) $(LIBDIR)/lib$(LIBRARY).a
	rm -f $(INCLUDEDIR)/bktypes.h $(INCLUDEDIR)/bikini.h

debug:
	@$(MAKE) BUILD_TYPE=debug

release:
	@$(MAKE) BUILD_TYPE=release

tests: $(TEST_BINS)

# The library test writes its temporary file into the current directory.
test: tests $(TARGET)
	@failed=0; \
	for t in $(TEST_BINS); do \
	    (cd $(TESTBINDIR) && ./$$(basename $$t)) || failed=1; \
	done; \
	sh $(TESTDIR)/test_cli.sh $(TARGET) || failed=1; \
	if [ $$failed -eq 0 ]; then echo "All tests passed."; else echo "Some tests FAILED."; exit 1; fi

help:
	@echo "Makefile for $(PROJECT) - Build targets:"
	@echo ""
	@echo "  make                        Build release version (default)."
	@echo "  make debug                  Build debug version."
	@echo "  make release                Build release version."
	@echo "  make clean                  Remove build artifacts."
	@echo "  make install                Install tool, library and headers to $(PREFIX)."
	@echo "  make install-strip          Install and strip the tool."
	@echo "  make uninstall              Remove the installed files."
	@echo "  make tests                  Build unit test binaries."
	@echo "  make test                   Build and run unit and CLI tests."
	@echo "  make help                   Show this help message."
	@echo ""
	@echo "Build options (set as environment variables or make arguments):"
	@echo ""
	@echo "  BUILD_TYPE=release|debug    Build type (default: release)."
	@echo "  SANITIZERS=0|1              Enable sanitizers (default: 0)."
	@echo "  VERBOSE_DEBUG=0|1           Enable verbose debug logging (default: 0)."
	@echo "  PREFIX=/path                Installation prefix (default: /usr/local)."
	@echo "  CC=compiler                 C compiler (default: cc)."

-include $(DEPS)
