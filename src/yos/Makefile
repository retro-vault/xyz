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
LD      :=  sdcc
OBJCOPY :=  sdobjcopy
TRUNC   :=  truncate

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

# Link against SDCC's Z80 runtime inside the Docker image.
# Provides: __divuint, __moduint, ___sdcc_call_hl, ___sdcc_call_iy, etc.
SDCC_Z80_LIB :=  /opt/sdcc/share/sdcc/lib/z80/z80.lib

LDFLAGS :=  -mz80 -Wl -y \
            --code-loc 0x0100 \
            --data-loc 0x5b00 \
            --no-std-crt0 \
            --nostdlib \
            -p \
            $(SDCC_Z80_LIB)

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

$(BIN_DIR)/$(TARGET).rom: $(BUILD_DIR)/$(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(BUILD_DIR)/$(TARGET).ihx $(BIN_DIR)/$(TARGET).rom
	$(TRUNC) -s 16K $(BIN_DIR)/$(TARGET).rom

$(BUILD_DIR)/$(TARGET).ihx: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $(BUILD_DIR)/$(@F) $<

$(BUILD_DIR)/%.rel: %.c
	$(CC) -c -o $(BUILD_DIR)/$(@F) $< $(CFLAGS)

$(BUILD_DIR)/%.rel: %.h

endif
