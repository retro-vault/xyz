# Assembly-only minimal YOS kernel for the ZX Spectrum ROM.

REPO_ROOT ?= $(realpath ../../..)
Y_ROOT ?= $(realpath ../..)
BUILD_DIR ?= $(REPO_ROOT)/build/yos-z80
DIST_DIR ?= $(REPO_ROOT)/bin/y
ZX_ROMS_DIR ?= $(DIST_DIR)/z80/spectrum/bin
BIN_DIR := $(ZX_ROMS_DIR)
TARGET := yos-kernel

X_DIST_DIR ?= $(REPO_ROOT)/bin/x
X_HOST_BIN_DIR ?= $(X_DIST_DIR)/bin
XAS := $(X_HOST_BIN_DIR)/xas
XLD := $(X_HOST_BIN_DIR)/xld
XAR := $(X_HOST_BIN_DIR)/xar
XCC := $(X_HOST_BIN_DIR)/xcc
XPROG := $(X_HOST_BIN_DIR)/xprog
XZ80 := $(X_DIST_DIR)/lib/libxz80.a
XRUNTIME := $(X_DIST_DIR)/z80/lib/libruntime.a
CXX ?= c++

XASFLAGS := --mode=sdcc
XLDFLAGS := --mode=sdcc -v -m -f bin -nostdlib \
	--no-default-runtime -T linker.lk

CRT := $(BUILD_DIR)/startup/crt0rom.rel
MAIN := $(BUILD_DIR)/main.rel
LIB_SOURCES := $(wildcard drivers/*.s) $(wildcard fs/*.s) \
	$(wildcard kernel/*.s) \
	$(filter-out startup/crt0rom.s,$(wildcard startup/*.s))
LIB_OBJECTS := $(addprefix $(BUILD_DIR)/,$(LIB_SOURCES:.s=.rel))
GPX_SOURCES := $(wildcard gpx/*.s)
GPX_OBJECTS := $(addprefix $(BUILD_DIR)/,$(GPX_SOURCES:.s=.rel))
GPX_INTEGRATION_OBJECTS := $(BUILD_DIR)/gpx/_gpx_name.rel \
	$(BUILD_DIR)/gpx/_gpx_service.rel
LOADER_OBJECTS := $(BUILD_DIR)/kernel/boot_shell.rel \
	$(BUILD_DIR)/kernel/process_load.rel \
	$(BUILD_DIR)/kernel/_crc32.rel \
	$(BUILD_DIR)/kernel/_process_read_exact.rel \
	$(BUILD_DIR)/kernel/_process_relocate.rel
ALL_OBJECTS := $(LIB_OBJECTS) $(GPX_OBJECTS)
ARCHIVE := $(BUILD_DIR)/libyos-kernel.lib
ARCHIVE_MANIFEST := $(BUILD_DIR)/libyos-kernel.members
OUTPUT := $(BIN_DIR)/$(TARGET).rom
SHELL_ASM := $(BUILD_DIR)/apps/shell.s
SHELL_OBJ := $(BUILD_DIR)/apps/shell.rel
SHELL_CRT := $(BUILD_DIR)/apps/shell-crt.rel
SHELL_XL := $(BUILD_DIR)/apps/shell.xl
SHELL_OUTPUT := $(BIN_DIR)/shell.sys
TEST_ROOT := $(BUILD_DIR)/test_roots.rel
TEST_OUTPUT := $(BUILD_DIR)/$(TARGET)-test.rom

.DEFAULT_GOAL := all

.PHONY: all clean test
all: $(OUTPUT) $(SHELL_OUTPUT)

test: $(OUTPUT) $(SHELL_OUTPUT) $(TEST_OUTPUT) $(BUILD_DIR)/test_kernel
	$(BUILD_DIR)/test_kernel $(TEST_OUTPUT) \
		$(BUILD_DIR)/$(TARGET)-test.map $(SHELL_OUTPUT)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OUTPUT) $(SHELL_OUTPUT) $(BIN_DIR)/$(TARGET).cdb

$(XAS) $(XLD) $(XAR) $(XCC) $(XPROG) $(XRUNTIME):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR)/%.rel: %.s $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) $< -o $@

$(ARCHIVE): $(ALL_OBJECTS) $(ARCHIVE_MANIFEST) $(XAR)
	rm -f $@
	$(XAR) rcs $@ @$(ARCHIVE_MANIFEST)

$(ARCHIVE_MANIFEST): $(ALL_OBJECTS)
	printf '%s\n' $(filter-out $(LOADER_OBJECTS),$(LIB_OBJECTS)) \
		| LC_ALL=C sort > $@
	# Discover the GPX service roots at the end of an archive scan. This lets
	# xld settle the kernel first and avoids wasting ROM around fixed traps.
	printf '%s\n' $(filter-out $(GPX_INTEGRATION_OBJECTS),$(GPX_OBJECTS)) \
		| LC_ALL=C sort >> $@
	printf '%s\n' $(GPX_INTEGRATION_OBJECTS) >> $@
	# Keep the boot loader after GPX so large graphics modules retain their
	# packing around the fixed divIDE/esxDOS ROM holes.
	printf '%s\n' $(LOADER_OBJECTS) >> $@
	test -s $@

$(OUTPUT): $(CRT) $(MAIN) $(ARCHIVE) $(XLD)
	mkdir -p $(BIN_DIR)
	$(XLD) $(XLDFLAGS) -Map=$(BUILD_DIR)/$(TARGET).map \
		-o $@ $(CRT) $(MAIN) $(ARCHIVE)
	@rom_end=$$(awk '$$2 == "s__GSFINAL" { print $$1 }' \
		$(BUILD_DIR)/$(TARGET).map); \
		test -n "$$rom_end" && test $$((0x$$rom_end)) -le 16384 || { \
			echo "YOS ROM content exceeds 16384 bytes (end 0x$$rom_end)" >&2; \
			exit 1; \
		}

$(SHELL_ASM): ../../tests/shell-yos/shell.c $(XCC)
	mkdir -p $(dir $@)
	$(XCC) --mode=sdcc -S -Os -std=c11 -I$(Y_ROOT)/include $< -o $@

$(SHELL_OBJ): $(SHELL_ASM) $(XAS)
	$(XAS) --mode=sdcc $< -o $@

$(SHELL_CRT): ../../tests/shell-yos/crt0.s $(XAS)
	mkdir -p $(dir $@)
	$(XAS) --mode=sdcc $< -o $@

$(SHELL_XL): $(SHELL_CRT) $(SHELL_OBJ) $(XLD) $(XRUNTIME)
	$(XLD) --mode=sdcc -nostdlib --no-default-runtime -e _entry \
		-o $@ $(SHELL_CRT) $(SHELL_OBJ) $(XRUNTIME)

$(SHELL_OUTPUT): $(SHELL_XL) $(XPROG)
	mkdir -p $(dir $@)
	$(XPROG) --process --name shell --stack-size 256 --min-os 8 \
		$< -o $@

$(TEST_ROOT): ../../tests/kernel-z80/test_roots.s $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) $< -o $@

$(TEST_OUTPUT): $(CRT) $(MAIN) $(TEST_ROOT) $(ARCHIVE) $(XLD)
	$(XLD) $(XLDFLAGS) -Map=$(BUILD_DIR)/$(TARGET)-test.map \
		-o $@ $(CRT) $(MAIN) $(TEST_ROOT) $(ARCHIVE)
	@rom_end=$$(awk '$$2 == "s__GSFINAL" { print $$1 }' \
		$(BUILD_DIR)/$(TARGET)-test.map); \
		test -n "$$rom_end" && test $$((0x$$rom_end)) -le 16384 || { \
			echo "YOS test ROM content exceeds 16384 bytes (end 0x$$rom_end)" >&2; \
			exit 1; \
		}

$(XZ80):
	$(MAKE) -C $(REPO_ROOT)/x/lib/xz80 \
		ROOT=$(REPO_ROOT)/x BUILD_DIR=$(REPO_ROOT)/build/x \
		DIST_DIR=$(X_DIST_DIR)

$(BUILD_DIR)/test_kernel: ../../tests/kernel-z80/test_kernel.cpp $(XZ80)
	mkdir -p $(BUILD_DIR)
	$(CXX) -std=c++20 -O2 -Wall -Wextra -pedantic \
		-I$(REPO_ROOT)/x/lib/xz80/include $< $(XZ80) -o $@
