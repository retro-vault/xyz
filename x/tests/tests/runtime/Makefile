# Makefile — xcc runtime Z80 emulation tests
#
# Build flow:
#   1. Assemble all runtime .s files with the selected assembler
#   2. Link at 0x0000 with xld, producing runtime.ihx + runtime.map
#   3. Convert to flat binary (tools/ihx2bin.py)
#   4. Generate C++ symbol header (tools/gen_symbols.py)
#   5. Compile & run C++ test binary (uses libxz80 from the repo)

_THIS_DIR    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
X_ROOT       ?= $(abspath $(_THIS_DIR)/../../..)
REPO_ROOT    ?= $(abspath $(X_ROOT)/..)
BUILD        := $(REPO_ROOT)/build/tests/runtime
RUNTIME_DIR  := $(X_ROOT)/runtime
XZ80_SRC     := $(X_ROOT)/lib/xz80/src
XZ80_INC     := $(X_ROOT)/lib/xz80/include

CXX         := g++
CXXFLAGS    := -std=c++20 -Wall -Wextra -g -I$(XZ80_INC) -I$(BUILD) -I.

# Default to xas now that the runtime REL output matches sdasz80 byte-for-byte.
# Keep both knobs overridable for parity debugging.
ASM         ?= $(REPO_ROOT)/bin/x/bin/xas
ASMFLAGS    ?= --mode=sdcc
LINK        ?= $(REPO_ROOT)/bin/x/bin/xld
LINKFLAGS   ?= --mode=sdcc --oformat=ihx -Ttext=0x0000 -nostdlib

# The direct runtime harness links with -nostdlib.  ieee16 contains thin
# libc-facing wrappers (for example ieee16_acos -> acosf), so keep this
# standalone image to the self-contained runtime helpers that it exercises.
RUNTIME_SRCS := $(shell find $(RUNTIME_DIR) -type f -name '*.s' ! -path '*/ieee16/*' | sort)
RUNTIME_RELS := $(patsubst $(RUNTIME_DIR)/%.s,$(BUILD)/objs/%.rel,$(RUNTIME_SRCS))
SUPPORT_SRCS := $(wildcard support/*.s)
SUPPORT_RELS := $(patsubst support/%.s,$(BUILD)/support/%.rel,$(SUPPORT_SRCS))

RUNTIME_IHX  := $(BUILD)/runtime.ihx
RUNTIME_MAP  := $(BUILD)/runtime.map
RUNTIME_BIN  := $(BUILD)/runtime.bin
SYMBOLS_HDR  := $(BUILD)/runtime_symbols.hpp

TEST_TARGET  := $(BUILD)/runtime_tests
XZ80_SRCS    := $(XZ80_SRC)/cpu.cpp $(XZ80_SRC)/memory.cpp \
                $(XZ80_SRC)/disassembler.cpp

.PHONY: all test clean

all: test

test: $(TEST_TARGET)
	$(TEST_TARGET) $(RUNTIME_BIN)

# Build test binary; depends on generated header and binary
$(TEST_TARGET): $(SYMBOLS_HDR) $(RUNTIME_BIN) test_main.cpp \
                test_int16.cpp test_int16_edge.cpp \
                test_shifts.cpp \
                test_int32.cpp test_int32_edge.cpp \
                test_float_arith.cpp test_float_conv.cpp test_float_edge.cpp \
                test_ll.cpp test_double.cpp \
                test_sys.cpp \
                runtime_machine.hpp float_helpers.hpp \
                runtime_symbols_future.hpp
	$(CXX) $(CXXFLAGS) $(XZ80_SRCS) test_main.cpp -o $@

# Generate C++ symbol header from linker map
# runtime.map is a side-effect of building runtime.ihx
$(SYMBOLS_HDR): $(RUNTIME_IHX)
	python3 tools/gen_symbols.py $(RUNTIME_MAP) $@

# Convert Intel HEX to flat binary
$(RUNTIME_BIN): $(RUNTIME_IHX)
	python3 tools/ihx2bin.py $< $@

# Link all assembled .rel files into one image
$(RUNTIME_IHX): $(RUNTIME_RELS) $(SUPPORT_RELS)
	$(LINK) $(LINKFLAGS) -Map=$(RUNTIME_MAP) -o $@ $^

# Assemble each .s file
$(BUILD)/objs/%.rel: $(RUNTIME_DIR)/%.s | $(BUILD)/objs
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD)/support/%.rel: support/%.s
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD)/objs:
	mkdir -p $@

$(BUILD):
	mkdir -p $@

$(SYMBOLS_HDR) $(RUNTIME_BIN) $(RUNTIME_IHX): | $(BUILD)

clean:
	rm -rf $(BUILD)
