# ---------------------------------------------------------------------------
# Makefile for yos (ZX Spectrum ROM).
# Invoked by the root Makefile or standalone: make (from this directory).
# Uses the staged X toolchain directly so the ROM build stays native on
# Linux, macOS, and Windows hosts.
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(ROOT)
Y_ROOT    ?= $(realpath ..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR ?= $(REPO_ROOT)/build/yos
DIST_DIR  ?= $(REPO_ROOT)/bin/y
ZX_ROMS_DIR ?= $(DIST_DIR)/z80/spectrum/bin
BIN_DIR   := $(ZX_ROMS_DIR)
TARGET :=  yos
X_DIST_DIR ?= $(REPO_ROOT)/bin/x
X_HOST_BIN_DIR ?= $(X_DIST_DIR)/bin
X_TARGET_LIB_DIR ?= $(X_DIST_DIR)/z80/lib

XCC := $(X_HOST_BIN_DIR)/xcc
XAS := $(X_HOST_BIN_DIR)/xas
XLD := $(X_HOST_BIN_DIR)/xld
XLIBC := $(X_TARGET_LIB_DIR)/libc.a
XRUNTIME := $(X_TARGET_LIB_DIR)/libruntime.a

.DEFAULT_GOAL := all

.PHONY: all
ifeq ($(YOS_DEBUG),1)
YOS_OUTPUTS := $(BIN_DIR)/$(TARGET).rom $(BIN_DIR)/$(TARGET).cdb
else
YOS_OUTPUTS := $(BIN_DIR)/$(TARGET).rom
endif
all: $(YOS_OUTPUTS)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN_DIR)/$(TARGET).rom $(BIN_DIR)/$(TARGET).cdb

.PHONY: docs
docs:
	pandoc -t gfm --template docs/00_template.gfm --toc -s --toc-depth=3 docs/*.md > README.md

YOS_OPT ?= -Os

XCCFLAGS := --mode=sdcc -S $(YOS_OPT) -std=c11 \
            -I$(THIS_DIR)/include \
            -I$(THIS_DIR) \
            -I$(Y_ROOT)/include

XASFLAGS := --mode=sdcc

# Protect the ZX Spectrum Interface 1 ROM paging trigger at 0x1708.
XLDFLAGS := --mode=sdcc -v -m -f bin \
              -nostdlib \
              --no-default-runtime \
              -x 0000-3FFF \
              -b _CODE=0100 \
              -b _DATA=5b00 \
              -r 1708-1708

ifeq ($(YOS_DEBUG),1)
XLDFLAGS += -g
endif

# Source search paths.
vpath %.c kernel
vpath %.c drivers
vpath %.c tty
vpath %.c apps
vpath %.s kernel
vpath %.s drivers
vpath %.s tty
vpath %.s apps

# CRT0 first, then all C and S sources.
CRT0   :=  crt0rom
C_SRCS :=  $(wildcard *.c) $(wildcard */*.c)
S_SRCS :=  $(filter-out startup/$(CRT0).s, $(wildcard *.s) $(wildcard */*.s))
C_ASMS := $(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.s,$(C_SRCS))))
C_OBJS := $(C_ASMS:.s=.rel)
S_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.s,%.rel,$(S_SRCS))))
# C and assembly sources are flattened into one build directory.  Keep a
# single object when a legacy .s file and a C file share the same basename.
S_OBJS := $(filter-out $(C_OBJS),$(S_OBJS))
OBJS   := $(BUILD_DIR)/$(CRT0).rel $(C_OBJS) $(S_OBJS)

$(XCC) $(XAS) $(XLD) $(XLIBC) $(XRUNTIME):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(YOS_OUTPUTS): $(OBJS) $(XLD) $(XLIBC) $(XRUNTIME) | $(BIN_DIR)
	$(XLD) $(XLDFLAGS) -o $(BIN_DIR)/$(TARGET).rom $(OBJS) $(XLIBC) $(XRUNTIME)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/%.s: %.c $(XCC) | $(BUILD_DIR)
	$(XCC) $(XCCFLAGS) -o $@ $<

$(BUILD_DIR)/%.rel: $(BUILD_DIR)/%.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/%.rel: %.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@
