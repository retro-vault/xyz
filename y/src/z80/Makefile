# Assembly-only minimal YOS kernel for the ZX Spectrum ROM.

REPO_ROOT ?= $(realpath ../../..)
Y_ROOT ?= $(realpath ../..)
BUILD_DIR ?= $(REPO_ROOT)/build/yos-z80
DIST_DIR ?= $(REPO_ROOT)/bin/y
RELEASE_DIR := $(DIST_DIR)
ARCH_DIR := $(RELEASE_DIR)/arch
BIN_48_DIR := $(ARCH_DIR)/48
BIN_128_DIR := $(ARCH_DIR)/128
BIN_NEXT_DIR := $(ARCH_DIR)/next
BIN_DIR := $(BIN_48_DIR)
RELEASE_INCLUDE_DIR := $(RELEASE_DIR)/include
RELEASE_SCRIPT_DIR := $(RELEASE_DIR)/scripts
RELEASE_SAMPLE_DIR := $(RELEASE_DIR)/samples
RELEASE_FIRMWARE_DIR := $(RELEASE_DIR)/firmware/esxdos089
ESXDOS_SOURCE_DIR := ../../third_party/esxdos089
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

YOS_BANK_BACKEND ?= 48
YOS_BANK_COUNT ?= 126

ifeq ($(filter $(YOS_BANK_BACKEND),48 128 next),)
$(error YOS_BANK_BACKEND must be 48, 128, or next)
endif
ifeq ($(shell test $(YOS_BANK_COUNT) -ge 1 -a $(YOS_BANK_COUNT) -le 126 && echo yes),)
$(error YOS_BANK_COUNT must be in the range 1..126)
endif

YOS_BANK_COUNT_128 := $(if $(filter 1 2 3 4 5 6,$(YOS_BANK_COUNT)),$(YOS_BANK_COUNT),6)
YOS_TEST_BANK_COUNT := $(if $(filter 48,$(YOS_BANK_BACKEND)),1,$(if \
	$(filter 128,$(YOS_BANK_BACKEND)),$(YOS_BANK_COUNT_128),$(YOS_BANK_COUNT)))
XASFLAGS := --mode=sdcc -DYOS_BANK_COUNT=$(YOS_BANK_COUNT) \
	-DYOS_BANK_COUNT_128=$(YOS_BANK_COUNT_128)
XLDFLAGS := --mode=sdcc -v -m -f bin -nostdlib \
	--no-default-runtime -T linker.lk

CRT := $(BUILD_DIR)/startup/crt0rom.rel
MAIN := $(BUILD_DIR)/main.rel
# Anchor descriptor helpers and the timer dispatcher early, fitting startup
# tightly around the fixed 04C6/0562 paging traps. Still linked only once.
EARLY_OBJECTS := $(BUILD_DIR)/fs/_esxdos_core.rel \
	$(BUILD_DIR)/kernel/_tmr_chain.rel \
	$(BUILD_DIR)/bank/common/_bank_data_rst30.rel \
	$(BUILD_DIR)/kernel/_yos_malloc.rel \
	$(BUILD_DIR)/kernel/_yos_free.rel \
	$(BUILD_DIR)/kernel/_yos_shrink.rel \
	$(BUILD_DIR)/kernel/_os_malloc.rel \
	$(BUILD_DIR)/kernel/_os_free.rel
LIB_SOURCES := $(wildcard drivers/*.s) $(wildcard fs/*.s) \
	$(wildcard bank/common/*.s) $(wildcard bank/48/*.s) \
	$(wildcard bank/128/*.s) $(wildcard bank/next/*.s) \
	$(wildcard kernel/*.s) \
	$(filter-out startup/crt0rom.s,$(wildcard startup/*.s))
LIB_OBJECTS := $(addprefix $(BUILD_DIR)/,$(LIB_SOURCES:.s=.rel))
BANK_MAP_OBJECTS := $(BUILD_DIR)/bank/48/_bank_map.rel \
	$(BUILD_DIR)/bank/128/_bank_map.rel \
	$(BUILD_DIR)/bank/next/_bank_map.rel
POLYGON_SOURCES := gpx/gpx_draw_polygon.s gpx/gpx_fill_polygon.s
GPX_SOURCES := $(filter-out $(POLYGON_SOURCES),$(wildcard gpx/*.s))
GPX_OBJECTS := $(addprefix $(BUILD_DIR)/,$(GPX_SOURCES:.s=.rel))
# Within one archive pass xld places members in this order. The two large
# loader modules and smaller helpers fill the space before the fixed
# 3CE1 vector/name reservation. Keep the cleanup and lookup helpers before
# the relatively large library finisher so they occupy the remaining gap.
LOADER_OBJECTS := $(BUILD_DIR)/fs/_esx_print.rel \
	$(BUILD_DIR)/bank/common/_bank_allocate_current.rel \
	$(BUILD_DIR)/kernel/_library_initialize.rel \
	$(BUILD_DIR)/kernel/boot_shell.rel \
	$(BUILD_DIR)/kernel/library_load.rel \
	$(BUILD_DIR)/kernel/_image_load.rel \
	$(BUILD_DIR)/kernel/_process_relocate.rel \
	$(BUILD_DIR)/kernel/_process_load_finish.rel \
	$(BUILD_DIR)/kernel/_image_retain.rel \
	$(BUILD_DIR)/kernel/_process_read_exact.rel \
	$(BUILD_DIR)/kernel/_library_find.rel \
	$(BUILD_DIR)/kernel/_crc32.rel \
	$(BUILD_DIR)/kernel/_process_has_threads.rel \
	$(BUILD_DIR)/kernel/_library_acquire.rel \
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
PRE_09F0_OBJECTS := $(BUILD_DIR)/kernel/_current_process.rel \
	$(BUILD_DIR)/fs/_directory_convert.rel \
	$(BUILD_DIR)/fs/_directory_validate.rel \
	$(BUILD_DIR)/kernel/thread_exit.rel \
	$(BUILD_DIR)/kernel/tmr_uninstall.rel \
	$(BUILD_DIR)/kernel/_yos_name.rel \
	$(BUILD_DIR)/kernel/process_load.rel
ORDERED_KERNEL_OBJECTS := $(BANK_MAP_OBJECTS) \
	$(foreach obj,$(sort $(filter-out $(BANK_MAP_OBJECTS) $(LOADER_OBJECTS) $(CRITICAL_ENTRY_OBJECTS) $(PRE_OPEN_OBJECTS) $(PRE_09F0_OBJECTS),$(LIB_OBJECTS))), \
	$(if $(filter %/tmr_install.rel,$(obj)),$(CRITICAL_ENTRY_OBJECTS)) \
	$(if $(filter %/fs/open.rel,$(obj)),$(PRE_OPEN_OBJECTS)) \
	$(obj)) \
	$(PRE_09F0_OBJECTS)
ARCHIVE := $(BUILD_DIR)/libyos-kernel.lib
ARCHIVE_MANIFEST := $(BUILD_DIR)/libyos-kernel.members
OUTPUT := $(BIN_DIR)/$(TARGET).rom
SHELL_XL := $(BUILD_DIR)/apps/shell.xl
SHELL_OUTPUT := $(BIN_DIR)/shell.sys
SHELL_ASM_OBJECT := $(BUILD_DIR)/apps/shell-asm.rel
SHELL_ASM_XL := $(BUILD_DIR)/apps/shell-asm.xl
SHELL_ASM_MAP := $(BUILD_DIR)/apps/shell-asm.map
SHELL_ASM_OUTPUT := $(BIN_DIR)/shell-asm.sys
LIBRARY_OBJECT := $(BUILD_DIR)/apps/shelllib.rel
LIBRARY_XL := $(BUILD_DIR)/apps/shelllib.xl
LIBRARY_OUTPUT := $(BUILD_DIR)/apps/shelllib.svc
DIST_HEADERS := $(addprefix $(RELEASE_INCLUDE_DIR)/,yos.h yos.inc)
DIST_SCRIPTS := $(addprefix $(RELEASE_DIR)/,run-48.sh run-128.sh run-next.sh) \
	$(RELEASE_SCRIPT_DIR)/run-yos.py
DIST_SOURCES := $(RELEASE_SAMPLE_DIR)/c/shell.c \
	$(RELEASE_SAMPLE_DIR)/c/Makefile \
	$(RELEASE_SAMPLE_DIR)/asm/shell.s \
	$(RELEASE_SAMPLE_DIR)/asm/linker.lk \
	$(RELEASE_SAMPLE_DIR)/asm/Makefile
ESXDOS_RELATIVE_FILES := $(shell cd $(ESXDOS_SOURCE_DIR) && \
	find . -type f -print | sed 's|^./||' | LC_ALL=C sort)
DIST_FIRMWARE := $(addprefix $(RELEASE_FIRMWARE_DIR)/,$(ESXDOS_RELATIVE_FILES))
DIST_FIRMWARE_DIRS := $(RELEASE_FIRMWARE_DIR)/TMP
DIST_README := $(RELEASE_DIR)/README.md
MODEL_BINARY_NAMES := $(TARGET).rom $(TARGET).rom.sha256 shell.sys \
	shell-asm.sys
DIST_MODEL_BINARIES := $(addprefix $(BIN_128_DIR)/,$(MODEL_BINARY_NAMES)) \
	$(addprefix $(BIN_NEXT_DIR)/,$(MODEL_BINARY_NAMES))

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

.PHONY: all clean test check-yos-abi distribution
all: $(OUTPUT) $(SHELL_OUTPUT) distribution

distribution: $(DIST_HEADERS) $(DIST_SCRIPTS) $(DIST_SOURCES) \
	$(DIST_FIRMWARE) $(DIST_FIRMWARE_DIRS) $(DIST_README) \
	$(OUTPUT).sha256 $(SHELL_ASM_OUTPUT) $(DIST_MODEL_BINARIES)
	rm -rf $(DIST_DIR)/z80 $(DIST_DIR)/bin $(DIST_DIR)/lib \
		$(DIST_DIR)/48 $(DIST_DIR)/128 $(DIST_DIR)/next \
		$(RELEASE_SAMPLE_DIR)/library
	rm -f $(RELEASE_INCLUDE_DIR)/dirent.h \
		$(RELEASE_SAMPLE_DIR)/c/shelllib.h \
		$(BIN_48_DIR)/shelllib.svc $(BIN_128_DIR)/shelllib.svc \
		$(BIN_NEXT_DIR)/shelllib.svc \
		$(RELEASE_SAMPLE_DIR)/c/shell.sys \
		$(RELEASE_SAMPLE_DIR)/asm/shell.sys

test: check-yos-abi $(OUTPUT) $(SHELL_OUTPUT) $(LIBRARY_OUTPUT) distribution \
		$(BUILD_DIR)/test_kernel
	python3 ../../tests/kernel-z80/test_rom_layout.py $(OUTPUT) \
		$(BUILD_DIR)/$(TARGET).map $(BUILD_DIR)/layout-tests
	$(BUILD_DIR)/test_kernel $(OUTPUT) \
		$(BUILD_DIR)/$(TARGET).map $(SHELL_OUTPUT) \
		$(LIBRARY_OUTPUT) $(BUILD_DIR)/apps/shelllib.map \
		$(YOS_BANK_BACKEND) $(YOS_TEST_BANK_COUNT)
	$(BUILD_DIR)/test_kernel $(OUTPUT) \
		$(BUILD_DIR)/$(TARGET).map $(SHELL_ASM_OUTPUT) \
		$(LIBRARY_OUTPUT) $(BUILD_DIR)/apps/shelllib.map \
		$(YOS_BANK_BACKEND) $(YOS_TEST_BANK_COUNT)
	python3 ../../tests/distribution/test_layout.py $(RELEASE_DIR)

check-yos-abi:
	python3 ../../scripts/check_yos_abi.py

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_48_DIR) $(BIN_128_DIR) $(BIN_NEXT_DIR)
	rm -rf $(RELEASE_INCLUDE_DIR) $(RELEASE_SCRIPT_DIR) \
		$(RELEASE_SAMPLE_DIR) $(RELEASE_DIR)/firmware
	rm -f $(DIST_README) $(addprefix $(RELEASE_DIR)/,run-48.sh run-128.sh run-next.sh)

$(XAS) $(XLD) $(XAR) $(XCC) $(XPROG) $(XRUNTIME):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR)/%.rel: %.s $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) -I../../include $< -o $@

$(ARCHIVE): $(ALL_OBJECTS) $(ARCHIVE_MANIFEST) $(XAR)
	rm -f $@
	$(XAR) rcs $@ @$(ARCHIVE_MANIFEST)

$(ARCHIVE_MANIFEST): $(ALL_OBJECTS) Makefile
	printf '%s\n' $(ORDERED_KERNEL_OBJECTS) > $@
	# Discover the graphics implementation roots at the end of an archive scan. This lets
	# xld settle the kernel first and avoids wasting ROM around fixed traps.
	printf '%s\n' $(GPX_OBJECTS) \
		| LC_ALL=C sort >> $@
	# Keep the boot loader after GPX so large graphics modules retain their
	# packing around the fixed divIDE/esxDOS ROM holes.
	printf '%s\n' $(LOADER_OBJECTS) >> $@
	test -s $@

$(OUTPUT): $(CRT) $(MAIN) $(ARCHIVE) $(XLD) Makefile ../../scripts/patch_rom.py
	mkdir -p $(BIN_DIR)
	rm -f $@
	$(XLD) $(XLDFLAGS) -Map=$(BUILD_DIR)/$(TARGET).map \
		-o $@ $(CRT) $(MAIN) $(EARLY_OBJECTS) $(ARCHIVE)
	python3 ../../scripts/patch_rom.py "$@" "$(BUILD_DIR)/$(TARGET).map"
	@rom_end=$$(awk '$$2 == "s__GSFINAL" { print $$1 }' \
		$(BUILD_DIR)/$(TARGET).map); \
		test -n "$$rom_end" && test $$((0x$$rom_end)) -le 16384 || { \
			echo "YOS ROM content exceeds 16384 bytes (end 0x$$rom_end)" >&2; \
			exit 1; \
		}

$(SHELL_XL): ../../samples/shell-c/shell.c \
		$(REPO_ROOT)/x/platforms/yos/include/yos.h $(XCC) $(XLD) $(XRUNTIME)
	mkdir -p $(dir $@)
	$(XCC) --mode=sdcc --platform=yos -Os -std=c11 $< -o $@

$(SHELL_OUTPUT): $(SHELL_XL) $(XPROG) Makefile
	mkdir -p $(dir $@)
	$(XPROG) --process --name shell --stack-size 512 --min-os 1 \
		$< -o $@

$(SHELL_ASM_OBJECT): ../../samples/shell-asm/shell.s ../../include/yos.inc $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) -I../../include $< -o $@

$(SHELL_ASM_XL): $(SHELL_ASM_OBJECT) $(XLD)
	$(XLD) --mode=sdcc -f xl -nostdlib --no-default-runtime \
		-T ../../samples/shell-asm/linker.lk -Map=$(SHELL_ASM_MAP) \
		-o $@ $<

$(SHELL_ASM_OUTPUT): $(SHELL_ASM_XL) $(XPROG) Makefile
	mkdir -p $(dir $@)
	$(XPROG) --process --name shell --stack-size 512 --min-os 1 \
		$< -o $@

$(LIBRARY_OBJECT): ../../tests/shell-yos/shelllib.s ../../include/yos.inc $(XAS)
	mkdir -p $(dir $@)
	$(XAS) $(XASFLAGS) -I../../include $< -o $@

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
		--export 0x$$(awk '$$2 == "_sum3" {print $$1}' $(BUILD_DIR)/apps/shelllib.map) \
		$< -o $@

$(OUTPUT).sha256: $(OUTPUT)
	test -f $@

$(BIN_128_DIR)/%: $(BIN_48_DIR)/%
	mkdir -p $(dir $@)
	cp $< $@

$(BIN_NEXT_DIR)/%: $(BIN_48_DIR)/%
	mkdir -p $(dir $@)
	cp $< $@

$(RELEASE_INCLUDE_DIR)/%: ../../include/%
	mkdir -p $(dir $@)
	cp $< $@

$(RELEASE_SCRIPT_DIR)/run-yos.py: ../../scripts/run_yos.py
	mkdir -p $(dir $@)
	install -m 755 $< $@

$(RELEASE_DIR)/run-%.sh: ../../scripts/run-%.sh
	mkdir -p $(dir $@)
	install -m 755 $< $@

$(RELEASE_SAMPLE_DIR)/c/%: ../../samples/shell-c/%
	mkdir -p $(dir $@)
	cp $< $@

$(RELEASE_SAMPLE_DIR)/asm/%: ../../samples/shell-asm/%
	mkdir -p $(dir $@)
	cp $< $@

$(RELEASE_FIRMWARE_DIR)/%: $(ESXDOS_SOURCE_DIR)/%
	mkdir -p $(dir $@)
	cp $< $@

$(RELEASE_FIRMWARE_DIR)/TMP:
	mkdir -p $@

$(DIST_README): ../../distribution/SPECTRUM.md
	mkdir -p $(dir $@)
	cp $< $@

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
