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
# Anchor descriptor helpers and the timer dispatcher early, fitting startup
# tightly around the fixed 04C6/0562 paging traps. Still linked only once.
EARLY_OBJECTS := $(BUILD_DIR)/fs/_esxdos_core.rel \
	$(BUILD_DIR)/kernel/_tmr_chain.rel
LIB_SOURCES := $(wildcard drivers/*.s) $(wildcard fs/*.s) \
	$(wildcard kernel/*.s) \
	$(filter-out startup/crt0rom.s,$(wildcard startup/*.s))
LIB_OBJECTS := $(addprefix $(BUILD_DIR)/,$(LIB_SOURCES:.s=.rel))
GPX_SOURCES := $(wildcard gpx/*.s)
GPX_OBJECTS := $(addprefix $(BUILD_DIR)/,$(GPX_SOURCES:.s=.rel))
GPX_INTEGRATION_OBJECTS := $(BUILD_DIR)/gpx/_gpx_name.rel \
	$(BUILD_DIR)/gpx/_gpx_service.rel
# Within one archive pass xld places members in this order. The two large
# loader modules and smaller helpers fill the space before the fixed
# 3CE1 vector/name reservation; the library finisher follows the 3D00 hole.
LOADER_OBJECTS := $(BUILD_DIR)/fs/_esx_print.rel \
	$(BUILD_DIR)/kernel/_library_initialize.rel \
	$(BUILD_DIR)/kernel/boot_shell.rel \
	$(BUILD_DIR)/kernel/process_load.rel \
	$(BUILD_DIR)/kernel/library_load.rel \
	$(BUILD_DIR)/kernel/_image_load.rel \
	$(BUILD_DIR)/kernel/_process_relocate.rel \
	$(BUILD_DIR)/kernel/_process_load_finish.rel \
	$(BUILD_DIR)/kernel/_image_retain.rel \
	$(BUILD_DIR)/kernel/_process_read_exact.rel \
	$(BUILD_DIR)/kernel/_library_find.rel \
	$(BUILD_DIR)/kernel/_crc32.rel \
	$(BUILD_DIR)/kernel/_library_acquire.rel \
	$(BUILD_DIR)/kernel/_process_has_threads.rel \
	$(BUILD_DIR)/kernel/_library_load_finish.rel
ALL_OBJECTS := $(LIB_OBJECTS) $(GPX_OBJECTS)
# Main declares these roots so they fill the gap before the 0562 trap.
CRITICAL_ENTRY_OBJECTS := $(BUILD_DIR)/startup/_critical_iff_repair.rel \
	$(BUILD_DIR)/startup/enter_critical_section.rel \
	$(BUILD_DIR)/kernel/_yos_malloc.rel \
	$(BUILD_DIR)/kernel/_image_transfer.rel
# The syscall table roots reap in the same pass as stat. Its compact body
# fits before the fixed 09F0 interrupt-return / print-entry reservation.
PRE_OPEN_OBJECTS := $(BUILD_DIR)/fs/stat.rel \
	$(BUILD_DIR)/kernel/_yos_install_timer.rel \
	$(BUILD_DIR)/kernel/_so_reap.rel
ORDERED_KERNEL_OBJECTS := $(foreach obj,$(sort $(filter-out $(LOADER_OBJECTS) $(CRITICAL_ENTRY_OBJECTS) $(PRE_OPEN_OBJECTS),$(LIB_OBJECTS))),\
	$(if $(filter %/tmr_install.rel,$(obj)),$(CRITICAL_ENTRY_OBJECTS))\
	$(if $(filter %/fs/open.rel,$(obj)),$(PRE_OPEN_OBJECTS))\
	$(obj))
ARCHIVE := $(BUILD_DIR)/libyos-kernel.lib
ARCHIVE_MANIFEST := $(BUILD_DIR)/libyos-kernel.members
OUTPUT := $(BIN_DIR)/$(TARGET).rom
OP_XL := $(BUILD_DIR)/apps/op.xl
OP_OUTPUT := $(BIN_DIR)/op.sys
LIBRARY_OBJECT := $(BUILD_DIR)/apps/shelllib.rel
LIBRARY_XL := $(BUILD_DIR)/apps/shelllib.xl
LIBRARY_OUTPUT := $(BIN_DIR)/shelllib.svc

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

.PHONY: all clean test
all: $(OUTPUT) $(OP_OUTPUT) $(LIBRARY_OUTPUT)

test: $(OUTPUT) $(OP_OUTPUT) $(LIBRARY_OUTPUT) $(BUILD_DIR)/test_kernel
	python3 ../../tests/kernel-z80/test_rom_layout.py $(OUTPUT) \
		$(BUILD_DIR)/$(TARGET).map $(BUILD_DIR)/layout-tests
	$(BUILD_DIR)/test_kernel $(OUTPUT) \
		$(BUILD_DIR)/$(TARGET).map $(OP_OUTPUT) \
		$(LIBRARY_OUTPUT) $(BUILD_DIR)/apps/shelllib.map

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OUTPUT) $(OP_OUTPUT) $(LIBRARY_OUTPUT) $(BIN_DIR)/$(TARGET).cdb $(OUTPUT).sha256

$(XAS) $(XLD) $(XAR) $(XCC) $(XPROG) $(XRUNTIME):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR)/%.rel: %.s $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) $< -o $@

$(ARCHIVE): $(ALL_OBJECTS) $(ARCHIVE_MANIFEST) $(XAR)
	rm -f $@
	$(XAR) rcs $@ @$(ARCHIVE_MANIFEST)

$(ARCHIVE_MANIFEST): $(ALL_OBJECTS) Makefile
	printf '%s\n' $(ORDERED_KERNEL_OBJECTS) > $@
	# Discover the GPX service roots at the end of an archive scan. This lets
	# xld settle the kernel first and avoids wasting ROM around fixed traps.
	printf '%s\n' $(filter-out $(GPX_INTEGRATION_OBJECTS),$(GPX_OBJECTS)) \
		| LC_ALL=C sort >> $@
	printf '%s\n' $(GPX_INTEGRATION_OBJECTS) >> $@
	# Keep the boot loader after GPX so large graphics modules retain their
	# packing around the fixed divIDE/esxDOS ROM holes.
	printf '%s\n' $(LOADER_OBJECTS) >> $@
	test -s $@

$(OUTPUT): $(CRT) $(MAIN) $(ARCHIVE) $(XLD) Makefile ../../scripts/patch_rom.py
	mkdir -p $(BIN_DIR)
	$(XLD) $(XLDFLAGS) -Map=$(BUILD_DIR)/$(TARGET).map \
		-o $@ $(CRT) $(MAIN) $(EARLY_OBJECTS) $(ARCHIVE)
	python3 ../../scripts/patch_rom.py "$@" "$(BUILD_DIR)/$(TARGET).map"
	@rom_end=$$(awk '$$2 == "s__GSFINAL" { print $$1 }' \
		$(BUILD_DIR)/$(TARGET).map); \
		test -n "$$rom_end" && test $$((0x$$rom_end)) -le 16384 || { \
			echo "YOS ROM content exceeds 16384 bytes (end 0x$$rom_end)" >&2; \
			exit 1; \
		}

$(OP_XL): ../../tests/shell-yos/shell.c ../../tests/shell-yos/shelllib.h \
		$(REPO_ROOT)/x/platforms/yos/include/yos.h $(XCC) $(XLD) $(XRUNTIME)
	mkdir -p $(dir $@)
	$(XCC) --mode=sdcc --platform=yos -Os -std=c11 $< -o $@

$(OP_OUTPUT): $(OP_XL) $(XPROG) Makefile
	mkdir -p $(dir $@)
	$(XPROG) --process --name op --stack-size 256 --min-os 1 \
		$< -o $@

$(LIBRARY_OBJECT): ../../tests/shell-yos/shelllib.s $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) $< -o $@

$(LIBRARY_XL): $(LIBRARY_OBJECT) $(XLD)
	$(XLD) --mode=sdcc -f xl -nostdlib --no-default-runtime \
		-T $(REPO_ROOT)/x/platforms/yos/linker.lk \
		-Map=$(BUILD_DIR)/apps/shelllib.map -o $@ $<

$(LIBRARY_OUTPUT): $(LIBRARY_XL) $(XPROG) Makefile
	mkdir -p $(dir $@)
	$(XPROG) --service --name shelllib --abi 1 --min-os 1 \
		--entry 0x$$(awk '$$2 == "_entry" {print $$1}' $(BUILD_DIR)/apps/shelllib.map) \
		--export 0x$$(awk '$$2 == "_probe" {print $$1}' $(BUILD_DIR)/apps/shelllib.map) \
		--export 0x$$(awk '$$2 == "_message" {print $$1}' $(BUILD_DIR)/apps/shelllib.map) \
		--export 0x$$(awk '$$2 == "_initializations" {print $$1}' $(BUILD_DIR)/apps/shelllib.map) \
		--export 0x$$(awk '$$2 == "_calls" {print $$1}' $(BUILD_DIR)/apps/shelllib.map) \
		$< -o $@

$(XZ80):
	$(MAKE) -C $(REPO_ROOT)/x/lib/xz80 \
		ROOT=$(REPO_ROOT)/x BUILD_DIR=$(REPO_ROOT)/build/x \
		DIST_DIR=$(X_DIST_DIR)

$(BUILD_DIR)/test_kernel: ../../tests/kernel-z80/test_kernel.cpp \
		../../tests/kernel-z80/mock_filesystem.h \
		../../tests/kernel-z80/test_libraries.h \
		../../tests/kernel-z80/test_thread_safety.h \
		../../tests/kernel-z80/test_circles.h $(XZ80)
	mkdir -p $(BUILD_DIR)
	$(CXX) -std=c++20 -O2 -Wall -Wextra -pedantic \
		-I$(REPO_ROOT)/x/lib/xz80/include $< $(XZ80) -o $@
