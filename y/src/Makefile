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

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BIN_DIR)/$(TARGET).rom $(BIN_DIR)/$(TARGET).cdb

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
	    -v $(REPO_ROOT):$(REPO_ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 \
	        ROOT=$(ROOT) \
	        REPO_ROOT=$(REPO_ROOT) \
	        Y_ROOT=$(Y_ROOT) \
	        X_ROOT=$(X_ROOT) \
	        BUILD_DIR=$(BUILD_DIR) \
	        DIST_DIR=$(DIST_DIR) \
	        ZX_ROMS_DIR=$(ZX_ROMS_DIR)

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
#   Y_ROOT/include   — yos.h and other shared Y headers
CFLAGS  :=  --std-c11 -mz80 --debug --nostdinc \
            -I$(THIS_DIR)/include \
            -I$(THIS_DIR) \
            -I$(Y_ROOT)/include

ASFLAGS :=  -xlos -g

XLD   :=  $(BUILD_DIR)/xld
XLD_SOURCES := $(wildcard $(X_ROOT)/src/xld/src/*.cpp)
XLD_HEADERS := $(wildcard $(X_ROOT)/src/xld/include/xld/*.h)
XBFD_INC  := $(X_ROOT)/lib/xbfd/include
XBFD_BUILD_DIR := $(BUILD_DIR)/host
XBFD_DIST_DIR := $(BUILD_DIR)/host-dist
XBFD_LIB  := $(XBFD_BUILD_DIR)/lib/libxbfd.a

XLD_CXXFLAGS := -std=c++2a -O2 \
                  -I$(X_ROOT)/src/xld/include \
                  -I$(Y_ROOT)/include \
                  -I$(XBFD_INC)

# SDCC Z80 runtime library used by xld for unresolved helper routines.
# Provides: __divuint, __moduint, ___sdcc_call_hl, ___sdcc_call_iy, etc.
SDCC_Z80_LIB :=  /opt/sdcc/share/sdcc/lib/z80/z80.lib
SDCC_XLD_LIB_DIR := $(BUILD_DIR)/sdcc-z80-lib
SDCC_XLD_LIB := $(SDCC_XLD_LIB_DIR)/z80.lib

# Protect the ZX Spectrum Interface 1 ROM paging trigger at 0x1708.
XLDFLAGS := -f bin \
              -x 0000-3FFF \
              -b _CODE=0100 \
              -b _DATA=5b00 \
              -r 1708-1708

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

.PHONY: all
all: $(BIN_DIR)/$(TARGET).rom

$(BIN_DIR)/$(TARGET).rom: $(OBJS) $(XLD) $(SDCC_XLD_LIB)
	$(XLD) $(XLDFLAGS) -g -o $@ $(OBJS) $(SDCC_XLD_LIB)

$(XBFD_LIB): $(wildcard $(X_ROOT)/lib/xbfd/binary/*.cpp) \
             $(wildcard $(X_ROOT)/lib/xbfd/debug_info/*.cpp) \
             $(wildcard $(X_ROOT)/lib/xbfd/lscript/*.cpp) \
             $(wildcard $(X_ROOT)/lib/xbfd/object/*.cpp) \
             $(wildcard $(X_ROOT)/lib/xbfd/include/xbfd/*.h)
	$(MAKE) -C $(X_ROOT)/lib/xbfd \
	    ROOT=$(X_ROOT) \
	    BUILD_DIR=$(XBFD_BUILD_DIR) \
	    DIST_DIR=$(XBFD_DIST_DIR) \
	    CXX=$(CXX)

$(XLD): $(XLD_SOURCES) $(XLD_HEADERS) $(XBFD_LIB)
	$(CXX) -o $@ $(XLD_SOURCES) $(XLD_CXXFLAGS) $(XBFD_LIB)

$(SDCC_XLD_LIB): $(SDCC_Z80_LIB)
	rm -rf $(SDCC_XLD_LIB_DIR)
	mkdir -p $(SDCC_XLD_LIB_DIR)
	cd $(SDCC_XLD_LIB_DIR) && ar x $(SDCC_Z80_LIB)
	cd $(SDCC_XLD_LIB_DIR) && ls -1 *.rel > $(SDCC_XLD_LIB)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(@F) $<

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $(BUILD_DIR)/$(@F) $< $(CFLAGS)

$(BUILD_DIR)/%.rel: %.h

endif
