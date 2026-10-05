# ls-toto — GNU ls replica in C11.
#
# Targets: all (default), debug, test, clean, install.
# Every output lives under build/ (binaries, objects) and build/tests/
# (unit-test binary and objects).

# --- Compiler selection ------------------------------------------------------
# Prefer gcc, else clang. On Windows, prefer MSYS2 UCRT64 gcc so Git Bash does
# not pick up a minimal MinGW without the expected headers.
CC := $(shell command -v gcc >/dev/null 2>&1 && echo gcc || echo clang)
EXE :=
ifeq ($(OS),Windows_NT)
  EXE := .exe
  ifneq ($(wildcard C:/msys64/ucrt64/bin/gcc.exe),)
    CC := C:/msys64/ucrt64/bin/gcc
  endif
endif

# --- Platform probe ----------------------------------------------------------
# Preprocess the four POSIX headers ls_toto_fs.c needs. If any is missing
# (native MinGW lacks pwd.h and grp.h), build the Win32 filesystem path,
# which only needs kernel32 (linked by default).
HASH := \#
POSIX_PROBE := $(shell printf '$(HASH)include <dirent.h>\n$(HASH)include <sys/stat.h>\n$(HASH)include <pwd.h>\n$(HASH)include <grp.h>\n' \
	| $(CC) -E -x c - >/dev/null 2>&1 && echo yes || echo no)

ifeq ($(POSIX_PROBE),yes)
  PLATFORM_DEFS :=
else
  PLATFORM_DEFS := -DLS_TOTO_WIN32_FS
endif

# --- Flags -------------------------------------------------------------------
# -Werror: warnings are build failures. ls-toto is built on two very
# different platforms (POSIX and Win32 filesystem paths); treating warnings
# as errors catches portability slips on whichever platform builds first.
# _XOPEN_SOURCE=700 exposes POSIX.1-2008 plus XSI (st_blocks, S_ISVTX).
BASE_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror \
	-D_XOPEN_SOURCE=700 -Iinclude $(PLATFORM_DEFS)

CFLAGS := $(BASE_CFLAGS) -O2

CFLAGS_DEBUG := $(BASE_CFLAGS) -g -O1 -fsanitize=address,undefined \
	-fno-omit-frame-pointer

LDFLAGS :=

# --- Layout ------------------------------------------------------------------
BUILD_DIR := build
TEST_BUILD_DIR := $(BUILD_DIR)/tests

BIN := $(BUILD_DIR)/ls-toto$(EXE)
DEBUG_BIN := $(BUILD_DIR)/ls-toto-debug$(EXE)
TEST_CORE := $(TEST_BUILD_DIR)/test_core$(EXE)

SRCS := src/main.c src/ls_toto_cli.c src/ls_toto_emit.c src/ls_toto_fs.c \
	src/ls_toto_sort.c src/ls_toto_format.c src/ls_toto_out.c \
	src/ls_toto_list.c

OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))

HDRS := $(wildcard include/*.h)

TEST_SRCS := tests/test_runner.c tests/test_out_stub.c \
	tests/test_cli_parse.c tests/test_sort_order.c \
	tests/test_format_mode.c tests/test_format_size.c \
	tests/test_fs_readdir.c
	
TEST_HDRS := $(wildcard tests/*.h)
TEST_OBJS := $(patsubst tests/%.c,$(TEST_BUILD_DIR)/%.o,$(TEST_SRCS))

# Pure modules only: no real I/O is linked into the unit tests.
TEST_LINKED_SRCS := src/ls_toto_cli.c src/ls_toto_sort.c src/ls_toto_format.c
TEST_LINKED_OBJS := $(patsubst src/%.c,$(TEST_BUILD_DIR)/%.o,$(TEST_LINKED_SRCS))

PREFIX_BIN := /usr/local/bin

.PHONY: all debug test clean install

all: $(BIN)

$(BIN): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Built straight from sources so release objects are never reused.
debug: $(DEBUG_BIN)

$(DEBUG_BIN): $(SRCS) $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS_DEBUG) -o $@ $(SRCS) $(LDFLAGS)

$(TEST_CORE): $(TEST_OBJS) $(TEST_LINKED_OBJS) $(HDRS) $(TEST_HDRS) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) $(TEST_LINKED_OBJS)

$(TEST_BUILD_DIR)/%.o: tests/%.c $(HDRS) $(TEST_HDRS) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BUILD_DIR)/%.o: src/%.c $(HDRS) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST_CORE)
	./$(TEST_CORE)

$(BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

clean:
	rm -f $(BUILD_DIR)/*.o $(TEST_BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/ls-toto $(BUILD_DIR)/ls-toto.exe
	rm -f $(BUILD_DIR)/ls-toto-debug $(BUILD_DIR)/ls-toto-debug.exe
	rm -f $(TEST_BUILD_DIR)/test_core $(TEST_BUILD_DIR)/test_core.exe

install: $(BIN)
	install -m 755 $(BIN) $(PREFIX_BIN)
