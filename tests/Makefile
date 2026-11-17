# Unit tests for maze-escape.  Usage (from the repo root or from tests/):
#   make -C tests test        build + run everything, write reports/unit.xml
#   make -C tests lint        compile src/ with -Wall -Wextra -Werror
#   make -C tests test SAN=0  disable AddressSanitizer / UBSan
# The tests link the REAL game sources (everything in src/ except main.c)
# against a mock of MiniLibX, so no X server / display is needed.

ROOT      := $(abspath ..)
CC        ?= cc
SAN       ?= 1
BUILD     := build
REPORTS   := reports
UNAME     := $(shell uname -s)

ifeq ($(UNAME),Darwin)
  OS_FLAG := -DMACOS
  MLX_DIR := $(ROOT)/libs/mlx-macos
else
  OS_FLAG := -DLINUX
  MLX_DIR := $(ROOT)/libs/mlx-linux
endif

INCLUDES  := -I$(MLX_DIR) -I$(ROOT)/libs/libft -I$(ROOT)/libs/libftprintf
CFLAGS    := -g -O0 -std=gnu99 -Wall -Wextra $(OS_FLAG) $(INCLUDES)
ifeq ($(SAN),1)
  CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
endif

GAME_SRC  := $(filter-out %/main.c,$(wildcard $(ROOT)/src/*/*.c))
TEST_SRC  := $(wildcard unit/*.c)
GAME_OBJ  := $(patsubst $(ROOT)/src/%.c,$(BUILD)/game/%.o,$(GAME_SRC))
TEST_OBJ  := $(patsubst unit/%.c,$(BUILD)/unit/%.o,$(TEST_SRC))
LIBS      := -L$(ROOT)/libs/libftprintf -lftprintf -L$(ROOT)/libs/libft -lft
BIN       := $(BUILD)/unit_tests

.PHONY: test unit lint libs clean
test: unit

libs:
	$(MAKE) -C $(ROOT)/libs/libft
	$(MAKE) -C $(ROOT)/libs/libftprintf

$(BUILD)/game/%.o: $(ROOT)/src/%.c $(ROOT)/src/header.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/unit/%.o: unit/%.c $(wildcard unit/*.h) $(ROOT)/src/header.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): libs $(GAME_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(GAME_OBJ) $(TEST_OBJ) $(LIBS) -o $@

unit: $(BIN)
	@mkdir -p $(REPORTS)
	MAZE_ROOT=$(ROOT) ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 \
	UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
	$(BIN) --junit $(REPORTS)/unit.xml

# Strict warning gate for the game's own sources.
lint:
	@set -e; for f in $(GAME_SRC) $(ROOT)/src/core/main.c; do \
	  echo "lint $$f"; \
	  $(CC) $(OS_FLAG) $(INCLUDES) -Wall -Wextra -Werror -fsyntax-only $$f; \
	done

clean:
	rm -rf $(BUILD) $(REPORTS)
