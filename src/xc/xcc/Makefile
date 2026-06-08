# Absolute path to this Makefile's directory — always correct regardless of
# where make is invoked from, and safe when ROOT/BUILD_DIR are already
# exported by the root Makefile (the ?= guards prevent overwriting them).
_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT      ?= $(abspath $(_THIS_DIR)/../../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR  ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin

CXX ?= g++

XBFD_LIB := $(DIST_DIR)/lib/libxbfd.a
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -I include -I src  -I$(ROOT)/include -I$(ROOT)/lib/xbfd/include
DBGFLAGS ?= -g3 -O0 -DDEBUG -fsanitize=address,undefined
RELFLAGS ?= -O2 -DNDEBUG
LDFLAGS ?=

BUILD    ?= debug
ifeq ($(BUILD),release)
  CXXFLAGS += $(RELFLAGS)
else
  CXXFLAGS += $(DBGFLAGS)
  LDFLAGS  += -fsanitize=address,undefined
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
DIST_INCDIR := $(DISTDIR)/include/z80

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
  $(SRCDIR)/backend/z80/z80peep.cpp \
  $(SRCDIR)/driver/options.cpp \
  $(SRCDIR)/driver/main.cpp

OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test check dist lint dirs bench

all: $(TARGET)

$(XBFD_LIB):
	$(MAKE) -C $(ROOT)/lib/xbfd

$(TARGET): $(OBJS) $(XBFD_LIB) | $(HOST_BIN_DIR)
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
	@bash tests/run_tests.sh $(TARGET)

# Run the repeatable codegen-only benchmark against SDCC.
# Outputs land under $(BUILD_DIR)/xc/xcc/bench/codegen/.
bench: $(TARGET)
	@bash tests/run_codegen_bench.sh $(TARGET)

# Like 'make test' but forces a debug build (sanitizers active).
# Use this before committing to catch memory and UB issues.
check:
	@$(MAKE) BUILD=debug test

# Stage deployment layout under $(DISTDIR).
# Copy $(DISTDIR)/bin/, $(DISTDIR)/lib/, and $(DISTDIR)/include/
# into /usr/local/ to install.
dist: $(TARGET)
	@command -v sdasz80 >/dev/null 2>&1 || { \
	  echo "xcc: error: sdasz80 not found in PATH — install z88dk to build the runtime"; \
	  exit 1; }
	@mkdir -p $(DIST_BINDIR) $(DIST_LIBDIR) $(DIST_INCDIR)
	strip -o $(DIST_BINDIR)/xcc $(TARGET)
	@rm -f $(DIST_LIBDIR)/*.rel
	@for src in $(RUNTIME_SOURCES); do \
	  rel="$(DIST_LIBDIR)/$$(basename "$${src%.s}").rel"; \
	  sdasz80 -o "$$rel" "$$src"; \
	done
	@cp -R $(ROOT)/lib/libc/include/. $(DIST_INCDIR)/
	@echo "Staged:"
	@echo "  $(DIST_BINDIR)/xcc         -> /usr/local/bin/xcc"
	@echo "  $(DIST_LIBDIR)/*.rel -> /usr/local/lib/xcc/runtime/"
	@echo "  $(DIST_INCDIR)        -> /usr/local/include/z80/"

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
