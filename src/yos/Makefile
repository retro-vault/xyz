# ---------------------------------------------------------------------------
# Makefile for yos (ZX Spectrum ROM).
# Invoked by the root Makefile or standalone: make (from this directory).
#
# Two modes:
#   Outside Docker (default): launches a single container that re-invokes
#                             this Makefile with IN_DOCKER=1.
#   Inside  Docker (IN_DOCKER=1): performs actual compilation.
#
# This keeps Docker startup overhead to one container per build rather than
# one container per source file.
# ---------------------------------------------------------------------------

ROOT     ?= $(realpath ../..)
THIS_DIR := $(realpath .)
BUILD_DIR := $(ROOT)/build/yos
BIN_DIR   := $(ROOT)/bin/yos

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: docs
docs:
	pandoc -t gfm --template docs/00_template.gfm --toc -s --toc-depth=3 docs/*.md > README.md

ifndef IN_DOCKER
# ---------------------------------------------------------------------------
# Host entry point: one docker run for the whole build.
# ---------------------------------------------------------------------------
.PHONY: all
all:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	docker run --rm \
	    -u $(shell id -u):$(shell id -g) \
	    -v $(ROOT):$(ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 ROOT=$(ROOT)

else
# ---------------------------------------------------------------------------
# Inside-Docker: actual compilation using tools from the container.
# ---------------------------------------------------------------------------

CC      :=  sdcc
AS      :=  sdasz80
CXX     :=  g++

# --nostdinc: use only our own headers, not SDCC's built-ins.
# Include paths:
#   THIS_DIR/include — custom stdbool.h, stdint.h, string.h, etc.
#   THIS_DIR         — kernel/, tty/, drivers/ headers (<kernel/foo.h>)
#   ROOT/include     — yos.h and other repo-level headers
CFLAGS  :=  --std-c11 -mz80 --debug --nostdinc \
            -I$(THIS_DIR)/include \
            -I$(THIS_DIR) \
            -I$(ROOT)/include

ASFLAGS :=  -xlos -g

XLINK   :=  $(BUILD_DIR)/xlink
XLINK_SOURCES := $(wildcard $(ROOT)/src/xc/xlink/src/*.cpp)
XLINK_HEADERS := $(wildcard $(ROOT)/src/xc/xlink/include/xlink/*.hpp)
XLINK_CXXFLAGS := -std=c++2a -O2 -I$(ROOT)/src/xc/xlink/include

# SDCC Z80 runtime library used by xlink for unresolved helper routines.
# Provides: __divuint, __moduint, ___sdcc_call_hl, ___sdcc_call_iy, etc.
SDCC_Z80_LIB :=  /opt/sdcc/share/sdcc/lib/z80/z80.lib
SDCC_XLINK_LIB_DIR := $(BUILD_DIR)/sdcc-z80-lib
SDCC_XLINK_LIB := $(SDCC_XLINK_LIB_DIR)/z80.lib

XLINKFLAGS := -f bin \
              -x 0000-3FFF \
              -b _CODE=0100 \
              -b _DATA=5b00 \
              -r 1700-170F

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
OBJS   :=  $(addprefix $(BUILD_DIR)/,$(notdir \
               $(CRT0).rel \
               $(patsubst %.c,%.rel,$(C_SRCS)) \
               $(patsubst %.s,%.rel,$(S_SRCS)) ))

TARGET :=  yos

.PHONY: all
all: $(BIN_DIR)/$(TARGET).rom

$(BIN_DIR)/$(TARGET).rom: $(OBJS) $(XLINK) $(SDCC_XLINK_LIB)
	$(XLINK) $(XLINKFLAGS) -n $(BUILD_DIR)/$(TARGET).noi -o $@ $(OBJS) $(SDCC_XLINK_LIB)

$(XLINK): $(XLINK_SOURCES) $(XLINK_HEADERS)
	$(CXX) -o $@ $(XLINK_SOURCES) $(XLINK_CXXFLAGS)

$(SDCC_XLINK_LIB): $(SDCC_Z80_LIB)
	rm -rf $(SDCC_XLINK_LIB_DIR)
	mkdir -p $(SDCC_XLINK_LIB_DIR)
	cd $(SDCC_XLINK_LIB_DIR) && ar x $(SDCC_Z80_LIB)
	cd $(SDCC_XLINK_LIB_DIR) && ls -1 *.rel > $(SDCC_XLINK_LIB)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(@F) $<

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $(BUILD_DIR)/$(@F) $< $(CFLAGS)

$(BUILD_DIR)/%.rel: %.h

endif
