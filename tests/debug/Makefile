ROOT      ?= $(realpath ../..)
BUILD_DIR := $(ROOT)/build/debug
DIST_DIR  ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
Z80_BIN_DIR ?= $(DIST_DIR)/z80/bin
Z80_APPS_DIR ?= $(Z80_BIN_DIR)/apps
BIN_DIR   := $(Z80_APPS_DIR)/debug
RUNTIME_DIR := $(ROOT)/src/xc/xcc/lib/runtime
RT_BUILD_DIR := $(BUILD_DIR)/runtime
XCC       := $(HOST_BIN_DIR)/xcc
XAS       := $(HOST_BIN_DIR)/xas
XAR       := $(HOST_BIN_DIR)/xar
XLD       := $(HOST_BIN_DIR)/xld
RUNTIME_SOURCES := $(shell find $(RUNTIME_DIR) -type f -name '*.s' | sort)
RUNTIME_RELS := $(patsubst $(RUNTIME_DIR)/%.s,$(RT_BUILD_DIR)/%.rel,$(RUNTIME_SOURCES))
RUNTIME_LIB := $(RT_BUILD_DIR)/runtime.lib

.DEFAULT_GOAL := all

XCCFLAGS ?= -S -g -O0
XASFLAGS ?= --mode=sdcc -g
XLDFLAGS ?= -f bin -e _entry -b _CODE=0100 -x 0100-0FFF -g

.PHONY: all clean

all: $(BIN_DIR)/sieve.bin

# Build host tools only when their own sources are newer than the binary.
$(XCC):
	$(MAKE) -C $(ROOT)/src/xc/xcc all

$(XAS):
	$(MAKE) -C $(ROOT)/src/xc/xas all

$(XAR):
	$(MAKE) -C $(ROOT)/src/xc/xar all

$(XLD):
	$(MAKE) -C $(ROOT)/src/xc/xld all

$(BUILD_DIR):
	mkdir -p $@

$(RT_BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/sieve.s: sieve.c | $(XCC) $(BUILD_DIR)
	$(XCC) $(XCCFLAGS) $< -o $@

$(BUILD_DIR)/sieve.rel: $(BUILD_DIR)/sieve.s | $(XAS)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/crt0.rel: crt0.s | $(XAS) $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(RT_BUILD_DIR)/%.rel: $(RUNTIME_DIR)/%.s | $(XAS) $(RT_BUILD_DIR)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) $< -o $@

$(RUNTIME_LIB): $(RUNTIME_RELS) | $(XAR) $(RT_BUILD_DIR)
	rm -f $@
	$(XAR) rcs $@ $(RUNTIME_RELS)

$(BIN_DIR)/sieve.bin: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/sieve.rel $(RUNTIME_LIB) | $(XLD) $(BIN_DIR)
	$(XLD) $(XLDFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
