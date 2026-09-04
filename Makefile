# ==============================================================================
# MLIB - Modular C Data Structures Library Makefile
# Cross-Platform: Linux, macOS, Windows (MinGW/MSYS2)
# ==============================================================================

CC ?= gcc
AR ?= ar
ARFLAGS = rcs

# Installation paths (POSIX standard)
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCDIR ?= $(PREFIX)/include/mlib

# Base Flags
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -I./include
LDFLAGS  ?=
PICFLAG  ?= -fPIC

# Package Metadata
PROJECT_NAME = mlib
VERSION      = 0.3.0
ARCHIVE_NAME = $(PROJECT_NAME)-v$(VERSION).tar.gz

# Sanitizers Configuration
ASAN_FLAGS     = -fsanitize=address
UBSAN_FLAGS    = -fsanitize=undefined
SANITIZE_FLAGS = $(ASAN_FLAGS) $(UBSAN_FLAGS) -fno-omit-frame-pointer

# Valgrind Configuration
VALGRIND       = valgrind
VALGRIND_FLAGS = --leak-check=full \
                 --show-leak-kinds=all \
                 --track-origins=yes \
                 --error-exitcode=1

# Directory Structure
BIN_DIR       = ./bin
BUILD_DIR     = ./build
OBJ_DIR       = ./build/objects
OBJ_TESTS_DIR = ./build/tests
SRC_DIR       = ./src
TESTS_DIR     = ./tests
INC_DIR       = ./include

# ------------------------------------------------------------------------------
# OS Detection & Target Artifacts
# ------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
	DETECTED_OS := Windows
	EXE_EXT     := .exe
	STATIC_LIB  := $(BIN_DIR)/lib$(PROJECT_NAME).a
	SHARED_LIB  := $(BIN_DIR)/$(PROJECT_NAME).dll
	TARGET      := $(BIN_DIR)/main.exe
else
	EXE_EXT :=
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		DETECTED_OS := macOS
		STATIC_LIB  := $(BIN_DIR)/lib$(PROJECT_NAME).a
		SHARED_LIB  := $(BIN_DIR)/lib$(PROJECT_NAME).dylib
		TARGET      := $(BIN_DIR)/main
	else
		DETECTED_OS := Linux
		STATIC_LIB  := $(BIN_DIR)/lib$(PROJECT_NAME).a
		SHARED_LIB  := $(BIN_DIR)/lib$(PROJECT_NAME).so
		TARGET      := $(BIN_DIR)/main
	endif
endif

# Source and Object Files
SRCS     = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS     = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

MAIN_SRC = main.c
MAIN_OBJ = $(OBJ_DIR)/main.o

# Header files
HDRS     = $(wildcard $(INC_DIR)/mlib/*.h)

# Format Targets
ALL_FORMAT_FILES = $(wildcard $(SRC_DIR)/*.c) \
                   $(wildcard $(INC_DIR)/*.h) \
                   $(wildcard $(INC_DIR)/*/*.h) \
                   $(wildcard $(TESTS_DIR)/*.c) \
                   $(wildcard $(TESTS_DIR)/*.h) \
                   $(wildcard *.c) \
                   $(wildcard *.h)

# ------------------------------------------------------------------------------
# ANSI Color & Style Definitions
# ------------------------------------------------------------------------------
C_RESET   = \033[0m
C_BOLD    = \033[1m
C_DIM     = \033[2m

C_RED     = \033[31m
C_GREEN   = \033[32m
C_YELLOW  = \033[33m
C_BLUE    = \033[34m
C_MAGENTA = \033[35m
C_CYAN    = \033[36m
C_WHITE   = \033[37m

# Status Prefixes
TAG_CC       = $(C_BLUE)$(C_BOLD)[ CC ]$(C_RESET)
TAG_AR       = $(C_MAGENTA)$(C_BOLD)[ AR ]$(C_RESET)
TAG_LD       = $(C_GREEN)$(C_BOLD)[ LD ]$(C_RESET)
TAG_RUN      = $(C_CYAN)$(C_BOLD)[ EXEC ]$(C_RESET)
TAG_MEMCHECK = $(C_YELLOW)$(C_BOLD)[ VALGRIND ]$(C_RESET)
TAG_SANITIZE = $(C_MAGENTA)$(C_BOLD)[ SANITIZE ]$(C_RESET)
TAG_FORMAT   = $(C_CYAN)$(C_BOLD)[ FORMAT ]$(C_RESET)
TAG_PACK     = $(C_MAGENTA)$(C_BOLD)[ PACK ]$(C_RESET)
TAG_DOC      = $(C_BLUE)$(C_BOLD)[ DOC ]$(C_RESET)
TAG_INSTALL  = $(C_GREEN)$(C_BOLD)[ INSTALL ]$(C_RESET)
TAG_UNINST   = $(C_RED)$(C_BOLD)[ UNINSTALL ]$(C_RESET)
TAG_CLEAN    = $(C_RED)$(C_BOLD)[ CLEAN ]$(C_RESET)

# ------------------------------------------------------------------------------
# Targets & Rules
# ------------------------------------------------------------------------------
.PHONY: all build debug release static shared run sanitize memcheck format pack doc install uninstall clean help FORCE

.DEFAULT_GOAL := help

FORCE:

help: ## Display this interactive help menu
	@printf "\n%bUsage:%b make %b[target]%b\n\n" "$(C_BOLD)$(C_WHITE)" "$(C_RESET)" "$(C_GREEN)" "$(C_RESET)"
	@printf "%bAvailable Targets:%b\n" "$(C_BOLD)$(C_CYAN)" "$(C_RESET)"
	@awk 'BEGIN {FS = ":.*?## "}; \
		/^[a-zA-Z0-9_-]+:.*?## / { \
			printf "  \033[32m\033[1m%-14s\033[0m \033[2m->\033[0m %s\n", $$1, $$2 \
		}' $(MAKEFILE_LIST)
	@printf "\n"

all: static shared $(TARGET) ## Build static/shared libraries and the demo binary

# Directory creation rules
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_TESTS_DIR):
	@mkdir -p $(OBJ_TESTS_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# ------------------------------------------------------------------------------
# Compilation Modes
# ------------------------------------------------------------------------------
debug: CFLAGS += -g3 -O0 -DDEBUG
debug: clean all ## Build with debug symbols and no optimizations

release: CFLAGS += -O3 -DNDEBUG
release: clean all ## Build with high optimization and assertions stripped

# ------------------------------------------------------------------------------
# Object Compilation
# ------------------------------------------------------------------------------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(OBJ_DIR)
	@printf "  %-20b %b%s%b -> %b%s%b\n" "$(TAG_CC)" "$(C_WHITE)" "$<" "$(C_RESET)" "$(C_CYAN)" "$@" "$(C_RESET)"
	@$(CC) $(CFLAGS) $(PICFLAG) -c $< -o $@

$(OBJ_DIR)/main.o: $(MAIN_SRC) $(HDRS) | $(OBJ_DIR)
	@printf "  %-20b %b%s%b -> %b%s%b\n" "$(TAG_CC)" "$(C_WHITE)" "$<" "$(C_RESET)" "$(C_CYAN)" "$@" "$(C_RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------------------------------------------
# Libraries & Binaries
# ------------------------------------------------------------------------------
static: $(STATIC_LIB) ## Build the static library (.a)

$(STATIC_LIB): $(OBJS) | $(BIN_DIR)
	@printf "  %-20b %b%s%b\n" "$(TAG_AR)" "$(C_BOLD)$(C_MAGENTA)" "$@" "$(C_RESET)"
	@$(AR) $(ARFLAGS) $@ $^
	@printf "%b✔ Static library built: %s%b\n" "$(C_GREEN)" "$@" "$(C_RESET)"

shared: $(SHARED_LIB) ## Build the shared library (.so / .dylib / .dll)

$(SHARED_LIB): $(OBJS) | $(BIN_DIR)
	@printf "  %-20b %b%s%b\n" "$(TAG_LD)" "$(C_BOLD)$(C_GREEN)" "$@" "$(C_RESET)"
ifeq ($(DETECTED_OS),macOS)
	@$(CC) -dynamiclib -o $@ $^ $(LDFLAGS)
else ifeq ($(DETECTED_OS),Windows)
	@$(CC) -shared -o $@ $^ $(LDFLAGS) -Wl,--out-implib,$(BIN_DIR)/lib$(PROJECT_NAME).dll.a
else
	@$(CC) -shared -o $@ $^ $(LDFLAGS)
endif
	@printf "%b✔ Shared library built: %s%b\n" "$(C_GREEN)" "$@" "$(C_RESET)"

build: $(TARGET) ## Compile all objects and link main binary

$(TARGET): $(OBJS) $(MAIN_OBJ) | $(BIN_DIR)
	@printf "  %-20b %b%s%b\n" "$(TAG_LD)" "$(C_BOLD)$(C_GREEN)" "$@" "$(C_RESET)"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@printf "%b✔ Main executable built: %s%b\n" "$(C_GREEN)" "$@" "$(C_RESET)"

# ==============================================================================
# Dynamic Unit Testing Infrastructure
# ==============================================================================

TEST_FILES      := $(wildcard $(TESTS_DIR)/mlib_test_*.c)
DATA_STRUCTURES := $(patsubst $(TESTS_DIR)/mlib_test_%.c, %, $(TEST_FILES))

.PHONY: test test-mem test-san

# ------------------------------------------------------------------------------
# 1. Regula Principală: make test
# ------------------------------------------------------------------------------
test: $(STATIC_LIB) FORCE ## Compile and run all available data structure tests
ifeq ($(strip $(DATA_STRUCTURES)),)
	@printf "%b⚠ Nu a fost găsit niciun fișier de test în %s (mlib_test_*.c)%b\n" "$(C_YELLOW)" "$(TESTS_DIR)" "$(C_RESET)"
else
	@for ds in $(DATA_STRUCTURES); do \
		$(MAKE) --no-print-directory test-$$ds || exit 1; \
	done
endif

# ------------------------------------------------------------------------------
# 2. Individual Rules: make test-<structura> (eg: make test-sll)
# ------------------------------------------------------------------------------
test-%: $(STATIC_LIB) $(HDRS) FORCE | $(BIN_DIR) $(OBJ_TESTS_DIR)
	@if [ ! -f "$(TESTS_DIR)/mlib_test_$*.c" ]; then \
		printf "%b[ SKIP ] Testul pentru '%s' nu există (%s/mlib_test_%s.c lipsește)%b\n" "$(C_YELLOW)" "$*" "$(TESTS_DIR)" "$*" "$(C_RESET)"; \
	else \
		printf "  %-20b %s/mlib_test_%s.c -> %s/mlib_test_%s.o\n" "$(TAG_CC)" "$(TESTS_DIR)" "$*" "$(OBJ_TESTS_DIR)" "$*"; \
		$(CC) $(CFLAGS) -c $(TESTS_DIR)/mlib_test_$*.c -o $(OBJ_TESTS_DIR)/mlib_test_$*.o && \
		printf "  %-20b %s/test_%s%s\n" "$(TAG_LD)" "$(BIN_DIR)" "$*" "$(EXE_EXT)"; \
		$(CC) $(CFLAGS) $(OBJ_TESTS_DIR)/mlib_test_$*.o $(STATIC_LIB) -o $(BIN_DIR)/test_$*$(EXE_EXT) $(LDFLAGS) && \
		printf "%b▶ Executing %s test suite...%b\n" "$(C_CYAN)$(C_BOLD)" "$*" "$(C_RESET)" && \
		printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)" && \
		$(BIN_DIR)/test_$*$(EXE_EXT) && \
		printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"; \
	fi

# ------------------------------------------------------------------------------
# 3. Valgrind Memory Check: make test-mem / make test-mem-<structura>
# ------------------------------------------------------------------------------
test-mem: CFLAGS += -g3 -O0
test-mem: clean $(STATIC_LIB) FORCE ## Run Valgrind on all available tests
ifeq ($(strip $(DATA_STRUCTURES)),)
	@printf "%b⚠ Nu există teste disponibile pentru analiza Valgrind.%b\n" "$(C_YELLOW)" "$(C_RESET)"
else
	@for ds in $(DATA_STRUCTURES); do \
		$(MAKE) --no-print-directory test-mem-$$ds CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" || exit 1; \
	done
endif

test-mem-%: CFLAGS += -g3 -O0
test-mem-%: $(STATIC_LIB) $(HDRS) FORCE | $(BIN_DIR) $(OBJ_TESTS_DIR)
	@if [ ! -f "$(TESTS_DIR)/mlib_test_$*.c" ]; then \
		printf "%b[ SKIP ] Valgrind skip: %s/mlib_test_%s.c nu a fost găsit%b\n" "$(C_YELLOW)" "$(TESTS_DIR)" "$*" "$(C_RESET)"; \
	else \
		$(CC) $(CFLAGS) -c $(TESTS_DIR)/mlib_test_$*.c -o $(OBJ_TESTS_DIR)/mlib_test_$*.o && \
		$(CC) $(CFLAGS) $(OBJ_TESTS_DIR)/mlib_test_$*.o $(STATIC_LIB) -o $(BIN_DIR)/test_$*$(EXE_EXT) $(LDFLAGS) && \
		printf "\n%b🔍 Valgrind Memory Check: %s...%b\n" "$(C_YELLOW)$(C_BOLD)" "$*" "$(C_RESET)" && \
		printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)" && \
		$(VALGRIND) $(VALGRIND_FLAGS) $(BIN_DIR)/test_$*$(EXE_EXT) && \
		printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"; \
	fi

# ------------------------------------------------------------------------------
# 4. Sanitizers (ASan & UBSan): make test-san / make test-san-<structura>
# ------------------------------------------------------------------------------
test-san: CFLAGS += $(SANITIZE_FLAGS) -g3 -O1
test-san: LDFLAGS += $(SANITIZE_FLAGS)
test-san: clean $(STATIC_LIB) FORCE ## Run ASan & UBSan on all available tests
ifeq ($(strip $(DATA_STRUCTURES)),)
	@printf "%b⚠ Nu există teste disponibile pentru verificarea cu sanitizere.%b\n" "$(C_YELLOW)" "$(C_RESET)"
else
	@for ds in $(DATA_STRUCTURES); do \
		$(MAKE) --no-print-directory test-san-$$ds CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" || exit 1; \
	done
endif

test-san-%: CFLAGS += $(SANITIZE_FLAGS) -g3 -O1
test-san-%: LDFLAGS += $(SANITIZE_FLAGS)
test-san-%: $(STATIC_LIB) $(HDRS) FORCE | $(BIN_DIR) $(OBJ_TESTS_DIR)
	@if [ ! -f "$(TESTS_DIR)/mlib_test_$*.c" ]; then \
		printf "%b[ SKIP ] Sanitizer skip: %s/mlib_test_%s.c nu a fost găsit%b\n" "$(C_YELLOW)" "$(TESTS_DIR)" "$*" "$(C_RESET)"; \
	else \
		$(CC) $(CFLAGS) -c $(TESTS_DIR)/mlib_test_$*.c -o $(OBJ_TESTS_DIR)/mlib_test_$*.o && \
		$(CC) $(CFLAGS) $(OBJ_TESTS_DIR)/mlib_test_$*.o $(STATIC_LIB) -o $(BIN_DIR)/test_$*$(EXE_EXT) $(LDFLAGS) && \
		printf "\n%b☣ Sanitizer Check (ASan/UBSan): %s...%b\n" "$(C_MAGENTA)$(C_BOLD)" "$*" "$(C_RESET)" && \
		printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)" && \
		$(BIN_DIR)/test_$*$(EXE_EXT) && \
		printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"; \
	fi

# ------------------------------------------------------------------------------
# Main Binary Execution & Checks
# ------------------------------------------------------------------------------
run: $(TARGET) ## Build and execute the main demo binary
	@printf "%b▶ Executing %s...%b\n" "$(C_CYAN)$(C_BOLD)" "$(TARGET)" "$(C_RESET)"
	@printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"
	@$(TARGET)
	@printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"

sanitize: CFLAGS += $(SANITIZE_FLAGS) -g3 -O1
sanitize: LDFLAGS += $(SANITIZE_FLAGS)
sanitize: clean $(TARGET) ## Compile and run main binary with ASan & UBSan
	@printf "\n%b☣ Running %s with ASan & UBSan...%b\n" "$(C_MAGENTA)$(C_BOLD)" "$(TARGET)" "$(C_RESET)"
	@printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"
	@$(TARGET)
	@printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"
	@printf "%b✔ Main binary sanitizer check passed!%b\n\n" "$(C_GREEN)$(C_BOLD)" "$(C_RESET)"

memcheck: CFLAGS += -g3 -O0
memcheck: clean $(TARGET) ## Run Valgrind leak check on main binary
	@printf "\n%b🔍 Running Valgrind Memory Analysis on %s...%b\n" "$(C_YELLOW)$(C_BOLD)" "$(TARGET)" "$(C_RESET)"
	@printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"
	@$(VALGRIND) $(VALGRIND_FLAGS) $(TARGET)
	@printf "%b--------------------------------------------------%b\n" "$(C_DIM)" "$(C_RESET)"
	@printf "%b✔ Main binary memory check completed without leaks!%b\n\n" "$(C_GREEN)$(C_BOLD)" "$(C_RESET)"

# ------------------------------------------------------------------------------
# Code Formatting & Documentation
# ------------------------------------------------------------------------------
format: ## Auto-format all source/header files with clang-format
	@if [ -n "$(strip $(ALL_FORMAT_FILES))" ]; then \
		printf "%b✨ Formatting project source files with clang-format...%b\n" "$(C_CYAN)$(C_BOLD)" "$(C_RESET)"; \
		clang-format -i -style=file $(ALL_FORMAT_FILES); \
		printf "%b✔ Formatting complete.%b\n" "$(C_GREEN)" "$(C_RESET)"; \
	else \
		printf "%b⚠ No source or header files found to format.%b\n" "$(C_YELLOW)" "$(C_RESET)"; \
	fi

doc: ## Generate Doxygen HTML documentation
	@if command -v doxygen >/dev/null 2>&1; then \
		printf "  %-20b Generating documentation...\n" "$(TAG_DOC)"; \
		doxygen Doxyfile 2>/dev/null || (doxygen -g >/dev/null 2>&1 && doxygen Doxyfile >/dev/null 2>&1); \
		printf "%b✔ Documentation generated in docs/html%b\n" "$(C_GREEN)" "$(C_RESET)"; \
	else \
		printf "%b⚠ Doxygen is not installed on this system.%b\n" "$(C_YELLOW)" "$(C_RESET)"; \
	fi

# ------------------------------------------------------------------------------
# Packaging & Installation
# ------------------------------------------------------------------------------
pack: clean ## Package clean source files into a tar.gz archive
	@printf "  %-20b Creating %b%s%b\n" "$(TAG_PACK)" "$(C_BOLD)$(C_WHITE)" "$(ARCHIVE_NAME)" "$(C_RESET)"
	@tar --exclude-vcs \
	     --exclude="*.tar.gz" \
	     --exclude="*.zip" \
	     -czf $(ARCHIVE_NAME) \
	     include src tests Makefile main.c README.md LICENSE .clang-format Doxyfile 2>/dev/null || \
	 tar --exclude-vcs \
	     --exclude="*.tar.gz" \
	     --exclude="*.zip" \
	     -czf $(ARCHIVE_NAME) \
	     include src tests Makefile main.c
	@printf "%b✔ Archive created: %s%b\n" "$(C_GREEN)" "$(ARCHIVE_NAME)" "$(C_RESET)"

install: static shared ## Install headers and libraries to system (POSIX)
	@printf "  %-20b Installing to %b%s%b\n" "$(TAG_INSTALL)" "$(C_BOLD)$(C_WHITE)" "$(PREFIX)" "$(C_RESET)"
	@mkdir -p $(LIBDIR)
	@mkdir -p $(INCDIR)
	@cp -f $(STATIC_LIB) $(LIBDIR)/
	@cp -f $(SHARED_LIB) $(LIBDIR)/ 2>/dev/null || true
	@cp -rf $(INC_DIR)/mlib/* $(INCDIR)/
	@printf "%b✔ Installation complete.%b\n" "$(C_GREEN)" "$(C_RESET)"

uninstall: ## Remove installed headers and libraries from system
	@printf "  %-20b Removing from %b%s%b\n" "$(TAG_UNINST)" "$(C_BOLD)$(C_WHITE)" "$(PREFIX)" "$(C_RESET)"
	@rm -f $(LIBDIR)/lib$(PROJECT_NAME).a
	@rm -f $(LIBDIR)/lib$(PROJECT_NAME).so
	@rm -f $(LIBDIR)/lib$(PROJECT_NAME).dylib
	@rm -rf $(INCDIR)
	@printf "%b✔ Uninstall complete.%b\n" "$(C_GREEN)" "$(C_RESET)"

# ------------------------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------------------------
clean: ## Remove all build artifacts, archives, and binaries
	@printf "  %-20b Removing %b%s%b and %b%s%b\n" "$(TAG_CLEAN)" "$(C_RED)" "$(BUILD_DIR)" "$(C_RESET)" "$(C_RED)" "$(BIN_DIR)" "$(C_RESET)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR) $(ARCHIVE_NAME) docs/
	@printf "%b✔ Cleanup complete.%b\n" "$(C_GREEN)" "$(C_RESET)"
