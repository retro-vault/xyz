# Makefile — xcc runtime Z80 emulation tests
#
# Build flow:
#   1. Assemble all runtime .s files with sdasz80
#   2. Link at 0x0000 with sdldz80, producing runtime.ihx + runtime.map
#   3. Convert to flat binary (tools/ihx2bin.py)
#   4. Generate C++ symbol header (tools/gen_symbols.py)
#   5. Compile & run C++ test binary (uses libxz80 from the repo)

ROOT        ?= ../..
BUILD       := $(ROOT)/build/tests/runtime
RUNTIME_DIR := $(ROOT)/src/xc/xcc/lib/runtime
XZ80_SRC    := $(ROOT)/lib/xz80/src
XZ80_INC    := $(ROOT)/lib/xz80/include

CXX         := g++
CXXFLAGS    := -std=c++20 -Wall -Wextra -g -I$(XZ80_INC) -I$(BUILD) -I.

ASM         := sdasz80
ASMFLAGS    :=
LINK        := sdldz80

RUNTIME_SRCS := $(shell find $(RUNTIME_DIR) -type f -name '*.s' | sort)
RUNTIME_RELS := $(patsubst $(RUNTIME_DIR)/%.s,$(BUILD)/objs/%.rel,$(RUNTIME_SRCS))

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
$(RUNTIME_IHX): $(RUNTIME_RELS)
	$(LINK) -n -m -j -i $@ -b _CODE=0x0000 $^ 2>/dev/null || \
	  $(LINK) -n -m -j -i $@ -b _CODE=0x0000 $^

# Assemble each .s file
$(BUILD)/objs/%.rel: $(RUNTIME_DIR)/%.s | $(BUILD)/objs
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD)/objs:
	mkdir -p $@

$(BUILD):
	mkdir -p $@

$(SYMBOLS_HDR) $(RUNTIME_BIN) $(RUNTIME_IHX): | $(BUILD)

clean:
	rm -rf $(BUILD)
