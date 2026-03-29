# ---------------------------------------------------------------------------
# Standalone Makefile for yos (ZX Spectrum ROM).
# Run directly from this directory: make
# Requires only Docker (wischner/sdcc-z80-zx-spectrum).
# Outputs go into build/ and bin/ inside this directory.
# ---------------------------------------------------------------------------

THIS_DIR    :=  $(realpath .)
BUILD_DIR   :=  $(THIS_DIR)/build
BIN_DIR     :=  $(THIS_DIR)/bin

# yos.h lives one level above src/yos in the repo's include/.
# Mount that alongside this directory so it is reachable inside the container.
XYZ_ROOT    :=  $(realpath ../..)

DOCKER_IMAGE :=  wischner/sdcc-z80-zx-spectrum:latest
DOCKER_RUN   :=  docker run --rm \
                 -u $(shell id -u):$(shell id -g) \
                 -v $(XYZ_ROOT):$(XYZ_ROOT) \
                 -w $$(pwd) \
                 $(DOCKER_IMAGE)

# ---------------------------------------------------------------------------
# 8-bit tools (all run inside the Docker container).
# ---------------------------------------------------------------------------
C8C      =  $(DOCKER_RUN) sdcc
AS8      =  $(DOCKER_RUN) sdasz80
LD8      =  $(DOCKER_RUN) sdcc
OBJCOPY  =  $(DOCKER_RUN) sdobjcopy
TRUNC    =  $(DOCKER_RUN) truncate

# ---------------------------------------------------------------------------
# Flags.
# --nostdinc: use only our own headers, not SDCC's built-ins.
# Include paths:
#   THIS_DIR/include — custom stdbool.h, stdint.h, string.h, etc.
#   THIS_DIR         — kernel/, tty/, drivers/ headers (<kernel/foo.h>)
#   XYZ_ROOT/include — yos.h
# ---------------------------------------------------------------------------
C8FLAGS  =  --std-c11 -mz80 --debug --nostdinc \
            -I$(THIS_DIR)/include \
            -I$(THIS_DIR) \
            -I$(XYZ_ROOT)/include

AS8FLAGS =  -xlos -g

# Link against SDCC's Z80 runtime inside the Docker image.
# Provides: __divuint, __moduint, ___sdcc_call_hl, ___sdcc_call_iy, etc.
SDCC_Z80_LIB := /opt/sdcc/share/sdcc/lib/z80/z80.lib

LD8FLAGS =  -mz80 -Wl -y \
            --code-loc 0x00ff \
            --data-loc 0x5b00 \
            --no-std-crt0 \
            --nostdlib \
            -p \
            $(SDCC_Z80_LIB)

# ---------------------------------------------------------------------------
# Source search paths.
# ---------------------------------------------------------------------------
vpath %.c kernel
vpath %.c drivers
vpath %.c tty
vpath %.c apps
vpath %.s kernel
vpath %.s drivers
vpath %.s tty
vpath %.s apps

# ---------------------------------------------------------------------------
# Source files and objects.
# test.c depends on libgpx which is not yet implemented — exclude it.
# ---------------------------------------------------------------------------
CRT0    =  crt0rom
C_SRCS  =  $(filter-out apps/test.c, $(wildcard *.c) $(wildcard */*.c))
S_SRCS  =  $(filter-out startup/$(CRT0).s, $(wildcard *.s) $(wildcard */*.s))
OBJS    =  $(addprefix $(BUILD_DIR)/,$(notdir \
               $(CRT0).rel \
               $(patsubst %.c,%.rel,$(C_SRCS)) \
               $(patsubst %.s,%.rel,$(S_SRCS)) ))

TARGET  =  yos

# ---------------------------------------------------------------------------
# Rules.
# ---------------------------------------------------------------------------
.PHONY: all
all: rom

.PHONY: rom
rom: mkdirs $(BIN_DIR)/$(TARGET).rom

.PHONY: mkdirs
mkdirs:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(TARGET).rom: $(BUILD_DIR)/$(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(basename $<).ihx $(basename $@).rom
	$(TRUNC) --size 16K $(basename $@).rom

$(BUILD_DIR)/$(TARGET).ihx: $(OBJS)
	$(LD8) $(LD8FLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/$(CRT0).rel: startup/$(CRT0).s
	$(AS8) $(AS8FLAGS) -o $@ $<

$(BUILD_DIR)/%.rel: %.s
	$(AS8) $(AS8FLAGS) $(BUILD_DIR)/$(@F) $<

$(BUILD_DIR)/%.rel: %.c
	$(C8C) -c -o $(BUILD_DIR)/$(@F) $< $(C8FLAGS)

$(BUILD_DIR)/%.rel: %.h

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: docs
docs:
	pandoc -t gfm --template docs/00_template.gfm --toc -s --toc-depth=3 docs/*.md > README.md
