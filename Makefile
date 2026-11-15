# ============================================================================
# fibtool - multi-module C project (Jenkins practice)
#
#   make                  release build (default)
#   make BUILD=debug      debug build
#   make BUILD=asan       AddressSanitizer + UBSan build
#   make test             build + run unit tests (writes JUnit XML)
#   make lint             static analysis with cppcheck
#   make install DESTDIR=/tmp/stage PREFIX=/usr
#   make dist             tar.gz package in artifacts/
#   make help             list targets
#
# Every BUILD flavour gets its own directory under artifacts/, so flavours can
# be built in parallel (e.g. Jenkins `parallel` stages) without colliding.
# ============================================================================

# ---- configuration (override on the command line or via environment) -------
# make predefines CC=cc, so `CC ?= gcc` would never apply; test the origin instead
ifeq ($(origin CC),default)
CC := gcc
endif
AR       ?= ar
BUILD    ?= release
# WERROR=1 turns warnings into errors (use in CI)
WERROR   ?= 0
PREFIX   ?= /usr/local
DESTDIR  ?=

NAME     := fibtool
SO_MAJOR := 1
SO_FULL  := 1.0.0

# ---- version metadata ------------------------------------------------------
# version.txt contains e.g. "Build 20".  Jenkins exports BUILD_NUMBER, which
# wins over the file (because of ?=).
FILE_BUILD   := $(shell sed -n 's/^Build[[:space:]]*\([0-9][0-9]*\).*/\1/p' version.txt 2>/dev/null)
BUILD_NUMBER ?= $(if $(FILE_BUILD),$(FILE_BUILD),0)
GIT_SHA      := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
VERSION      := 1.0.$(BUILD_NUMBER)

# ---- directories -----------------------------------------------------------
ROOT     := artifacts
OUT      := $(ROOT)/$(BUILD)
OBJ_DIR  := $(OUT)/obj
BIN_DIR  := $(OUT)/bin
LIB_DIR  := $(OUT)/lib
GEN_DIR  := $(OUT)/gen
REP_DIR  := $(OUT)/test-reports

# ---- flags per build flavour ----------------------------------------------
BASE_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2
ifeq ($(WERROR),1)
BASE_CFLAGS += -Werror
endif

ifeq ($(BUILD),debug)
  FLAVOUR_CFLAGS := -O0 -g3 -DDEBUG
else ifeq ($(BUILD),release)
  FLAVOUR_CFLAGS := -O2 -g -DNDEBUG -D_FORTIFY_SOURCE=2 -fstack-protector-strong
else ifeq ($(BUILD),asan)
  FLAVOUR_CFLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
  FLAVOUR_LDFLAGS := -fsanitize=address,undefined
else
  $(error Unknown BUILD '$(BUILD)'. Use release, debug or asan)
endif

CPPFLAGS := -Iinclude -I$(GEN_DIR) -D_POSIX_C_SOURCE=200809L -DBUILD_FLAVOUR=\"$(BUILD)\"
CFLAGS   := $(BASE_CFLAGS) $(FLAVOUR_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS  := $(FLAVOUR_LDFLAGS) $(EXTRA_LDFLAGS)

# ---- artifacts -------------------------------------------------------------
CORE_SRCS := fibonacci parse primes          # -> libfibcore.a   (static)
UTIL_SRCS := fibutil                         # -> libfibutil.so  (shared)

CORE_OBJS := $(CORE_SRCS:%=$(OBJ_DIR)/%.o)
UTIL_OBJS := $(UTIL_SRCS:%=$(OBJ_DIR)/%.o)
MAIN_OBJ  := $(OBJ_DIR)/main.o
TEST_OBJS := $(OBJ_DIR)/tests/test_main.o $(OBJ_DIR)/tests/testfw.o \
             $(OBJ_DIR)/tests/test_fibonacci.o $(OBJ_DIR)/tests/test_primes.o \
             $(OBJ_DIR)/tests/test_parse.o

LIB_STATIC := $(LIB_DIR)/libfibcore.a
LIB_SO     := $(LIB_DIR)/libfibutil.so.$(SO_FULL)
LIB_SO_LNK := $(LIB_DIR)/libfibutil.so
VERSION_H  := $(GEN_DIR)/version.h
BIN        := $(BIN_DIR)/$(NAME)
TEST_BIN   := $(BIN_DIR)/run_tests
JUNIT_XML  := $(REP_DIR)/junit.xml

# ---- top-level targets -----------------------------------------------------
.DEFAULT_GOAL := all
.PHONY: all test lint install dist clean distclean help FORCE

all: $(BIN)

# Main program: static core lib + shared util lib. rpath=$ORIGIN/../lib lets the
# binary find libfibutil.so without LD_LIBRARY_PATH when run from bin/.
$(BIN): $(MAIN_OBJ) $(LIB_STATIC) $(LIB_SO_LNK) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(MAIN_OBJ) $(LIB_STATIC) -L$(LIB_DIR) -lfibutil \
	    -Wl,-rpath,'$$ORIGIN/../lib' -o $@

$(LIB_STATIC): $(CORE_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_SO): $(UTIL_OBJS) | $(LIB_DIR)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libfibutil.so.$(SO_MAJOR) -o $@ $^

$(LIB_SO_LNK): $(LIB_SO)
	ln -sf libfibutil.so.$(SO_FULL) $(LIB_DIR)/libfibutil.so.$(SO_MAJOR)
	ln -sf libfibutil.so.$(SO_MAJOR) $@

# Objects that go into the shared library need position-independent code.
$(UTIL_OBJS): CFLAGS += -fPIC

# Generic compile rules. -MMD -MP writes <obj>.d files listing header
# dependencies, so editing a .h rebuilds exactly what depends on it.
$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR) $(VERSION_H)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.c | $(OBJ_DIR)/tests $(VERSION_H)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Itests -MMD -MP -c $< -o $@

# ---- generated header ------------------------------------------------------
# Rewritten only when its content changes, so a new git SHA or build number
# triggers recompilation of just the files that include version.h.
$(VERSION_H): version.txt FORCE | $(GEN_DIR)
	@printf '#ifndef FIBTOOL_VERSION_H\n#define FIBTOOL_VERSION_H\n' > $@.tmp
	@printf '#define FIBTOOL_VERSION_STRING "%s+%s (%s, %s)"\n' \
	    "$(VERSION)" "$(GIT_SHA)" "$(BUILD)" "$(CC)" >> $@.tmp
	@printf '#endif\n' >> $@.tmp
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; echo "GEN   $@"; fi

FORCE:

# ---- tests -----------------------------------------------------------------
$(TEST_BIN): $(TEST_OBJS) $(LIB_STATIC) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(TEST_OBJS) $(LIB_STATIC) -o $@

test: $(TEST_BIN) $(BIN) | $(REP_DIR)
	$(TEST_BIN) $(JUNIT_XML)
	@echo "JUnit report: $(JUNIT_XML)"
	@# smoke test of the real binary
	@$(BIN) -n 20 | grep -q '^Fibonacci Series up to 20: 0, 1, 2, 3, 5, 8, 13$$'
	@$(BIN) -p -n 30 | grep -q '29$$'
	@echo "smoke tests passed"

# ---- static analysis -------------------------------------------------------
lint:
	@command -v cppcheck >/dev/null 2>&1 || { echo "cppcheck not installed"; exit 127; }
	cppcheck --enable=warning,style,performance,portability --error-exitcode=1 \
	    --inline-suppr --std=c11 -Iinclude -I$(GEN_DIR) -q src tests

# ---- install / package -----------------------------------------------------
install: all
	install -D -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(NAME)
	install -D -m 0755 $(LIB_SO) $(DESTDIR)$(PREFIX)/lib/libfibutil.so.$(SO_FULL)
	ln -sf libfibutil.so.$(SO_FULL) $(DESTDIR)$(PREFIX)/lib/libfibutil.so.$(SO_MAJOR)
	ln -sf libfibutil.so.$(SO_MAJOR) $(DESTDIR)$(PREFIX)/lib/libfibutil.so
	install -D -m 0644 include/fibutil.h $(DESTDIR)$(PREFIX)/include/fibutil.h

STAGE   := $(ROOT)/stage-$(BUILD)
PACKAGE := $(ROOT)/$(NAME)-$(VERSION)-$(GIT_SHA)-$(BUILD).tar.gz

dist: all
	rm -rf $(STAGE)
	$(MAKE) --no-print-directory install DESTDIR=$(CURDIR)/$(STAGE) PREFIX=/usr
	tar -C $(STAGE) -czf $(PACKAGE) .
	rm -rf $(STAGE)
	@echo "Package: $(PACKAGE)"

# ---- housekeeping ----------------------------------------------------------
$(OBJ_DIR) $(OBJ_DIR)/tests $(BIN_DIR) $(LIB_DIR) $(GEN_DIR) $(REP_DIR):
	mkdir -p $@

clean:
	rm -rf $(OUT)

distclean:
	rm -rf $(ROOT)

help:
	@sed -n '3,12p' Makefile | sed 's/^# \{0,1\}//'

# pull in the auto-generated header dependency files (silently skip if absent)
-include $(wildcard $(OBJ_DIR)/*.d $(OBJ_DIR)/tests/*.d)
