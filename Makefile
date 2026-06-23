# ---------------------------------------------------------------------------
# Root Makefile — delegates only, compiles nothing here.
# Add new sub-projects to SUBDIRS when they become ready to build.
#
# Usage:
#   make        — build everything
#   make clean  — remove all build artifacts
# ---------------------------------------------------------------------------

# Linux only.
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Prerequisite checks.
$(if $(shell which docker),,$(error "docker not found. Please install Docker."))
$(if $(shell which gcc),,$(error "gcc not found. Please install gcc."))

# Use the latest reachable vX.Y.Z tag as the default package version.
GIT_VERSION_TAG := $(shell git describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)

# Repository root and shared output directories (exported for sub-makes).
export PACKAGE_NAME       ?= x
ifneq ($(filter v%,$(GIT_VERSION_TAG)),)
export PACKAGE_VERSION    ?= $(patsubst v%,%,$(GIT_VERSION_TAG))
else
export PACKAGE_VERSION    ?= 0.1.0
endif
export PACKAGE_RELEASE    ?= 1
export VSIX_VERSION       ?= $(PACKAGE_VERSION)
export ROOT               := $(realpath .)
export BUILD_DIR          := $(ROOT)/build
export OUT_DIR            := $(ROOT)/bin
export X_DIST_DIR         := $(OUT_DIR)/x
export Y_DIST_DIR         := $(OUT_DIR)/y
export Z_DIST_DIR         := $(OUT_DIR)/z
export X_ROOT            := $(ROOT)/x
export Y_ROOT            := $(ROOT)/y
export Z_ROOT            := $(ROOT)/z
export PLATFORM           ?= none
export DEFAULT_PLATFORM   := $(PLATFORM)
export ZX_RAM_STORAGE     ?= plus3
export LIBC_PROFILE       ?= full
ifeq ($(LIBC_PROFILE),tiny)
override LIBC_FLOAT     := 0
override LIBC_DOUBLE    := 0
override LIBC_LONG      := 0
override LIBC_LONGLONG  := 0
else
LIBC_FLOAT     ?= 1
LIBC_DOUBLE    ?= 1
LIBC_LONG      ?= 1
LIBC_LONGLONG  ?= 1
endif
export LIBC_FLOAT LIBC_DOUBLE LIBC_LONG LIBC_LONGLONG

ifeq ($(ZX_RAM_STORAGE),plus3)
ZX_RAM_STORAGE_DEFINES := -DZX_RAM_PLUS3=1 -DZX_RAM_IDE=0
else ifeq ($(ZX_RAM_STORAGE),ide)
ZX_RAM_STORAGE_DEFINES := -DZX_RAM_PLUS3=0 -DZX_RAM_IDE=1
else
$(error ZX_RAM_STORAGE must be plus3 or ide)
endif

LIBC_FEATURE_DEFINES := \
	-D__XCC_LIBC_FLOAT=$(LIBC_FLOAT) \
	-D__XCC_LIBC_DOUBLE=$(LIBC_DOUBLE) \
	-D__XCC_LIBC_LONG=$(LIBC_LONG) \
	-D__XCC_LIBC_LONGLONG=$(LIBC_LONGLONG)
ifeq ($(LIBC_FLOAT),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_FLOAT=1
endif
ifeq ($(LIBC_DOUBLE),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_DOUBLE=1
endif
ifeq ($(LIBC_LONG),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_LONG=1
endif
ifeq ($(LIBC_LONGLONG),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_LONGLONG=1
endif
TARGET_FEATURE_DEFINES := $(LIBC_FEATURE_DEFINES) $(ZX_RAM_STORAGE_DEFINES)
TARGET_C8FLAGS := -I$(X_ROOT)/libc/include -I$(X_ROOT)/platforms/include $(TARGET_FEATURE_DEFINES)
TARGET_AS8FLAGS := --mode=sdcc $(TARGET_FEATURE_DEFINES)
export TARGET_FEATURE_DEFINES TARGET_C8FLAGS TARGET_AS8FLAGS

# GNU cross-toolchain prefix layout:
#   bin/, lib/, include/  — host executables, host libraries, host headers
#   z80/lib, z80/include  — target payload (libc, runtime, crt0, linker scripts)
export DIST_DIR           := $(X_DIST_DIR)
export HOST_BIN_DIR       := $(DIST_DIR)/bin
export PUBLIC_INCLUDE_DIR := $(DIST_DIR)/include
export PUBLIC_LIB_DIR     := $(DIST_DIR)/lib
export TARGET_DIR         := $(DIST_DIR)/z80
export TARGET_INCLUDE_DIR := $(TARGET_DIR)/include
export TARGET_LIB_DIR     := $(TARGET_DIR)/lib
export Z80_DIST_DIR       := $(Z_DIST_DIR)/z80
export Z80_BIN_DIR        := $(Z80_DIST_DIR)/bin
export ZX_TARGET_DIR      := $(Z80_DIST_DIR)/spectrum
export ZX_TARGET_BIN_DIR  := $(ZX_TARGET_DIR)/bin
export ZX_TARGET_INCLUDE_DIR := $(ZX_TARGET_DIR)/include
export ZX_TARGET_LIB_OUT_DIR := $(ZX_TARGET_DIR)/lib
export ZX_ROMS_DIR        := $(Y_DIST_DIR)/z80/spectrum/bin
export ZX_APPS_DIR        := $(ZX_TARGET_BIN_DIR)/apps
export ZX_MDR_DIR         := $(ZX_TARGET_BIN_DIR)/mdr
export SHARE_DIR          := $(DIST_DIR)/share
export DOCS_DIR           := $(SHARE_DIR)/doc
export PKG_DIR            := $(DIST_DIR)/pkg
export VSIX_STAGE_DIR     := $(PKG_DIR)/vsix

.PHONY: all x clean help
.PHONY: stage-includes stage-target-assets stage-xcc-support
.PHONY: stage-layout-cleanup stage-toolchain-targets
.PHONY: stage-toolchain-prefixes stage-dist-docs

all:
	@echo "==> $(X_ROOT)"
	@$(MAKE) -C $(X_ROOT) \
		REPO_ROOT=$(ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(X_DIST_DIR) \
		OUT_DIR=$(OUT_DIR) \
		Z_DIST_DIR=$(Z_DIST_DIR) \
		PLATFORM=$(PLATFORM) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) \
		ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) \
		LIBC_PROFILE=$(LIBC_PROFILE) \
		LIBC_FLOAT=$(LIBC_FLOAT) \
		LIBC_DOUBLE=$(LIBC_DOUBLE) \
		LIBC_LONG=$(LIBC_LONG) \
		LIBC_LONGLONG=$(LIBC_LONGLONG)
	@echo "==> $(Y_ROOT)"
	@$(MAKE) -C $(Y_ROOT) \
		REPO_ROOT=$(ROOT) \
		X_ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(Y_DIST_DIR) \
		HOST_BIN_DIR=$(Y_DIST_DIR)/bin \
		PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib \
		PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include
	@echo "==> packages"
	@$(MAKE) -C $(X_ROOT)/pkg all REPO_ROOT=$(ROOT) X_ROOT=$(X_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) VSIX_STAGE_DIR=$(VSIX_STAGE_DIR)

x:
	@$(MAKE) -C $(X_ROOT) x \
		REPO_ROOT=$(ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(X_DIST_DIR) \
		OUT_DIR=$(OUT_DIR) \
		Z_DIST_DIR=$(Z_DIST_DIR) \
		PLATFORM=$(PLATFORM) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) \
		ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) \
		LIBC_PROFILE=$(LIBC_PROFILE) \
		LIBC_FLOAT=$(LIBC_FLOAT) \
		LIBC_DOUBLE=$(LIBC_DOUBLE) \
		LIBC_LONG=$(LIBC_LONG) \
		LIBC_LONGLONG=$(LIBC_LONGLONG)

stage-includes:
	@$(MAKE) -C $(X_ROOT) stage-includes REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR)

stage-target-assets:
	@$(MAKE) -C $(X_ROOT) stage-target-assets REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR)

stage-xcc-support:
	@$(MAKE) -C $(X_ROOT) stage-xcc-support REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR) PLATFORM=$(PLATFORM) DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) LIBC_PROFILE=$(LIBC_PROFILE) LIBC_FLOAT=$(LIBC_FLOAT) LIBC_DOUBLE=$(LIBC_DOUBLE) LIBC_LONG=$(LIBC_LONG) LIBC_LONGLONG=$(LIBC_LONGLONG)

stage-layout-cleanup:
	@$(MAKE) -C $(X_ROOT) stage-layout-cleanup REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR) PLATFORM=$(PLATFORM)

stage-toolchain-targets:
	@:

stage-toolchain-prefixes:
	@:

stage-dist-docs:
	@$(MAKE) -C $(X_ROOT) stage-dist-docs REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR)

help:
	@printf '%s\n' \
		'Usage: make [target] [VARIABLE=value ...]' \
		'' \
		'Targets:' \
		'  all                  Build x tools, y tools, and packages (default).' \
		'  x                    Build only the x compiler suite distribution.' \
		'  stage-includes       Stage host and target headers into bin/x.' \
		'  stage-target-assets  Stage target output directories and assets.' \
		'  stage-xcc-support    Build and stage runtime, libc, crt0, and linker files.' \
		'  stage-dist-docs      Stage x distribution documentation.' \
		'  clean                Remove bin/ and build/ outputs.' \
		'  help                 Show this help.' \
		'' \
		'Common variables:' \
		'  PLATFORM=none|cpm3|emu          Default target platform (default: none).' \
		'  ZX_RAM_STORAGE=plus3|ide        ZX RAM storage backend (default: plus3).' \
		'  LIBC_PROFILE=full|tiny          Libc feature profile (default: full).' \
		'  LIBC_FLOAT=0|1                  Enable float libc/runtime support.' \
		'  LIBC_DOUBLE=0|1                 Enable double libc/runtime support.' \
		'  LIBC_LONG=0|1                   Enable long libc/runtime support.' \
		'  LIBC_LONGLONG=0|1               Enable long long libc/runtime support.' \
		'  PACKAGE_NAME=name               Package name (default: x).' \
		'  PACKAGE_VERSION=x.y.z           Package version (default: latest vX.Y.Z tag).' \
		'  PACKAGE_RELEASE=n               Package release number (default: 1).'

clean:
	rm -rf $(OUT_DIR) $(BUILD_DIR)
	@$(MAKE) -C $(X_ROOT)/pkg clean REPO_ROOT=$(ROOT) X_ROOT=$(X_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) VSIX_STAGE_DIR=$(VSIX_STAGE_DIR)
