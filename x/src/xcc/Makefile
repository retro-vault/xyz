# Absolute path to this Makefile's directory — always correct regardless of
# where make is invoked from, and safe when ROOT/BUILD_DIR are already
# exported by the root Makefile (the ?= guards prevent overwriting them).
_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT      ?= $(abspath $(_THIS_DIR)/../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR  ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin

CXX ?= g++
XAS ?= $(HOST_BIN_DIR)/xas
XASFLAGS ?= --mode=sdcc

XBFD_LIB := $(DIST_DIR)/lib/libxbfd.a
XOPT_DIR := $(ROOT)/lib/xopt
XOPT_INC := $(XOPT_DIR)/include
XOPT_LIB := $(BUILD_DIR)/lib/libxopt.a
XOPT_SRCS := $(wildcard $(XOPT_DIR)/src/*.cpp $(XOPT_DIR)/include/xopt/*.h)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -I include -I src  -I$(ROOT)/platforms/include -I$(ROOT)/lib/xbfd/include -I$(XOPT_INC)
DBGFLAGS ?= -g3 -O0 -DDEBUG
SANITIZE_FLAGS ?= -fsanitize=address,undefined
RELFLAGS ?= -O2 -DNDEBUG
LDFLAGS ?=
HOST_OS ?= $(shell uname -s 2>/dev/null || echo unknown)
HOST_TRIPLE ?= $(shell $(CXX) -dumpmachine 2>/dev/null || echo unknown)
ENABLE_SANITIZERS ?= auto
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
XCC_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
CXXFLAGS += -DXCC_VERSION=\"$(XCC_VERSION)\"

ifeq ($(ENABLE_SANITIZERS),auto)
  ENABLE_SANITIZERS := 1
  # MinGW/MSYS CI environments commonly lack the sanitizer runtime libs.
  ifneq ($(filter Windows_NT MINGW% MSYS% CYGWIN%,$(OS) $(HOST_OS)),)
    ENABLE_SANITIZERS := 0
  endif
  ifneq ($(findstring mingw,$(HOST_TRIPLE)),)
    ENABLE_SANITIZERS := 0
  endif
endif

# Normal builds should produce a redistributable compiler binary.
# Use BUILD=debug or `make check` when you want sanitizer instrumentation.
BUILD    ?= release
ifeq ($(BUILD),release)
  CXXFLAGS += $(RELFLAGS)
else
  CXXFLAGS += $(DBGFLAGS)
  ifeq ($(ENABLE_SANITIZERS),1)
    CXXFLAGS += $(SANITIZE_FLAGS)
    LDFLAGS  += $(SANITIZE_FLAGS)
  endif
endif

SRCDIR   := src
PROJECT_BUILD_DIR := $(BUILD_DIR)/xc/xcc
OBJDIR   := $(PROJECT_BUILD_DIR)/obj
TARGET   := $(HOST_BIN_DIR)/xcc
RUNTIME_DIR := lib/runtime
RUNTIME_SOURCES := $(shell find $(RUNTIME_DIR) -type f -name '*.s' | sort)
DISTDIR := $(PROJECT_BUILD_DIR)/dist
DIST_BINDIR := $(DISTDIR)/bin
DIST_LIBDIR := $(DISTDIR)/lib/xcc/runtime
DIST_INCDIR := $(DISTDIR)/z80/include

SRCS := \
  $(SRCDIR)/frontend/diag.cpp \
  $(SRCDIR)/frontend/preproc.cpp \
  $(SRCDIR)/frontend/lexer.cpp \
  $(SRCDIR)/frontend/types.cpp \
  $(SRCDIR)/frontend/symtab.cpp \
  $(SRCDIR)/frontend/const_eval.cpp \
  $(SRCDIR)/frontend/parser.cpp \
  $(SRCDIR)/frontend/parser_declspec.cpp \
  $(SRCDIR)/frontend/parser_declarator.cpp \
  $(SRCDIR)/frontend/parser_stmt.cpp \
  $(SRCDIR)/frontend/parser_init.cpp \
  $(SRCDIR)/frontend/parser_expr.cpp \
  $(SRCDIR)/frontend/sema.cpp \
  $(SRCDIR)/ir/icode.cpp \
  $(SRCDIR)/opt/iromod.cpp \
  $(SRCDIR)/opt/iropt.cpp \
  $(SRCDIR)/ir/irgen.cpp \
  $(SRCDIR)/ir/irgen_decl.cpp \
  $(SRCDIR)/ir/irgen_stmt.cpp \
  $(SRCDIR)/ir/irgen_expr.cpp \
  $(SRCDIR)/ir/irgen_lvalue.cpp \
  $(SRCDIR)/ir/irgen_init.cpp \
  $(SRCDIR)/backend/sdasz80_emitter.cpp \
  $(SRCDIR)/backend/gnuas_emitter.cpp \
  $(SRCDIR)/backend/z80/debug_info_adapter.cpp \
  $(SRCDIR)/backend/z80/z80gen.cpp \
  $(SRCDIR)/backend/z80/z80gen_convention.cpp \
  $(SRCDIR)/backend/z80/z80gen_data.cpp \
  $(SRCDIR)/backend/z80/z80gen_operand.cpp \
  $(SRCDIR)/backend/z80/z80gen_ctrl.cpp \
  $(SRCDIR)/backend/z80/z80gen_mem.cpp \
  $(SRCDIR)/backend/z80/z80gen_arith.cpp \
  $(SRCDIR)/backend/z80/z80gen_regalloc.cpp \
  $(SRCDIR)/driver/options.cpp \
  $(SRCDIR)/driver/main.cpp

OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test test-exec test-external test-c23 test-all check dist lint dirs bench test-unified

TEST_RUNNER := $(ROOT)/tests/run_tests.sh
LEGACY_TEST_DIR := $(ROOT)/../archive/x/tests/tests/xcc-legacy

all: $(TARGET)

$(XBFD_LIB):
	$(MAKE) -C $(ROOT)/lib/xbfd

$(XOPT_LIB): $(XOPT_SRCS)
	$(MAKE) -C $(XOPT_DIR) ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR)

$(TARGET): $(OBJS) $(XOPT_LIB) $(XBFD_LIB) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

dirs:
	@mkdir -p $(sort $(dir $(OBJS)))

$(HOST_BIN_DIR):
	@mkdir -p $@

-include $(DEPS)

# Run the test suite against the current build.
test: $(TARGET)
	@bash $(TEST_RUNNER) $(TARGET) --filter xcc

test-exec: $(TARGET)
	@bash $(TEST_RUNNER) $(TARGET) --filter xcc_exec_

test-external: $(TARGET)
	@bash $(TEST_RUNNER) $(TARGET) --filter xcc_external_

test-c23: $(TARGET)
	@bash $(TEST_RUNNER) $(TARGET) --filter xcc_external_c23_

test-all: test test-exec test-external test-c23 test-unified

# Run the new unified xemu-backed suite.
test-unified: $(TARGET)
	@bash $(TEST_RUNNER) $(TARGET) --filter xcc

# Run the repeatable codegen-only benchmark against SDCC.
# Outputs land under $(BUILD_DIR)/xc/xcc/bench/codegen/.
bench: $(TARGET)
	@bash $(LEGACY_TEST_DIR)/run_codegen_bench.sh $(TARGET)

# Like 'make test' but forces a debug build (sanitizers active when supported).
# Use this before committing to catch memory and UB issues.
check:
	@$(MAKE) BUILD=debug test

# Stage deployment layout under $(DISTDIR).
# Copy $(DISTDIR)/bin/, $(DISTDIR)/lib/, and $(DISTDIR)/include/
# into /usr/local/ to install.
dist: $(TARGET)
	@command -v $(XAS) >/dev/null 2>&1 || { \
	  echo "xcc: error: xas not found — build x/src/xas first"; \
	  exit 1; }
	@mkdir -p $(DIST_BINDIR) $(DIST_LIBDIR) $(DIST_INCDIR)
	strip -o $(DIST_BINDIR)/xcc $(TARGET)
	@rm -f $(DIST_LIBDIR)/*.rel
	@for src in $(RUNTIME_SOURCES); do \
	  rel="$(DIST_LIBDIR)/$$(basename "$${src%.s}").rel"; \
	  $(XAS) $(XASFLAGS) "$$src" -o "$$rel"; \
	done
	@cp -R $(ROOT)/libc/include/. $(DIST_INCDIR)/
	@echo "Staged:"
	@echo "  $(DIST_BINDIR)/xcc         -> /usr/local/bin/xcc"
	@echo "  $(DIST_LIBDIR)/*.rel -> /usr/local/lib/xcc/runtime/"
	@echo "  $(DIST_INCDIR)        -> /usr/local/z80/include/"

# Static analysis. Requires cppcheck in PATH; set LINT=1 to enable.
# Example: make lint  or  LINT=1 make lint
lint:
	@command -v cppcheck >/dev/null 2>&1 || { \
	  echo "xcc: lint: cppcheck not found — install it with: sudo apt install cppcheck"; \
	  exit 0; }
	cppcheck --enable=warning,style,performance \
	         --std=c++17 \
	         --suppress=missingIncludeSystem \
	         -I include -I src \
	         $(SRCDIR) 2>&1

clean:
	rm -rf $(PROJECT_BUILD_DIR)
	rm -f $(TARGET)
