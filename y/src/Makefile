# Build both YOS implementations from their independent source trees.

REPO_ROOT ?= $(realpath ../..)
Y_ROOT ?= $(realpath ..)
BUILD_DIR ?= $(REPO_ROOT)/build/yos
DIST_DIR ?= $(REPO_ROOT)/bin/y
ZX_ROMS_DIR ?= $(DIST_DIR)/z80/spectrum/bin

.DEFAULT_GOAL := all

.PHONY: all c z80 clean
all: c z80

c:
	$(MAKE) -C c REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		ZX_ROMS_DIR=$(ZX_ROMS_DIR)

z80:
	$(MAKE) -C z80 REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)-z80 DIST_DIR=$(DIST_DIR) \
		ZX_ROMS_DIR=$(ZX_ROMS_DIR)

clean:
	$(MAKE) -C c REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		ZX_ROMS_DIR=$(ZX_ROMS_DIR) clean
	$(MAKE) -C z80 REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)-z80 DIST_DIR=$(DIST_DIR) \
		ZX_ROMS_DIR=$(ZX_ROMS_DIR) clean
