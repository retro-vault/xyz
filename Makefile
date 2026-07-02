# ---------------------------------------------------------------------------
# Root Makefile — delegates only, compiles nothing here.
# Add new sub-projects to SUBDIRS when they become ready to build.
#
# Usage:
#   make        — build everything
#   make clean  — remove all build artifacts
# ---------------------------------------------------------------------------

# Use the latest reachable vX.Y.Z tag as the default package version.
GIT_VERSION_TAG := $(shell git describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)

include x/mk/model.mk

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
LIBC_FLOAT     ?= $(X_MODEL_LIBC_FLOAT_DEFAULT)
LIBC_DOUBLE    ?= $(X_MODEL_LIBC_DOUBLE_DEFAULT)
LIBC_LONG      ?= $(X_MODEL_LIBC_LONG_DEFAULT)
LIBC_LONGLONG  ?= $(X_MODEL_LIBC_LONGLONG_DEFAULT)
LIBC_STDIO_FLOAT ?= $(X_MODEL_LIBC_STDIO_FLOAT_DEFAULT)
ifeq ($(LIBC_FLOAT),0)
override LIBC_STDIO_FLOAT := 0
endif
export X_MODEL X_MODEL_NAME X_MODEL_SUFFIX
export LIBC_FLOAT LIBC_DOUBLE LIBC_LONG LIBC_LONGLONG LIBC_STDIO_FLOAT

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
	-D__XCC_LIBC_LONGLONG=$(LIBC_LONGLONG) \
	-D__XCC_LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT)
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
ifeq ($(LIBC_STDIO_FLOAT),0)
LIBC_FEATURE_DEFINES += -D__XCC_LIBC_NO_STDIO_FLOAT=1
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

.PHONY: all packages x x-s x-m x-l x-models clean help
.PHONY: test-x-s test-x-m test-x-l test-x-models
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
		LIBC_LONGLONG=$(LIBC_LONGLONG) \
		LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT) \
		X_MODEL=$(X_MODEL)
	@echo "==> $(Y_ROOT)"
	@$(MAKE) -C $(Y_ROOT) \
		REPO_ROOT=$(ROOT) \
		X_ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(Y_DIST_DIR) \
		HOST_BIN_DIR=$(Y_DIST_DIR)/bin \
		PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib \
		PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include

packages:
	@echo "==> packages"
	@$(MAKE) -C $(X_ROOT)/pkg all REPO_ROOT=$(ROOT) X_ROOT=$(X_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) VSIX_STAGE_DIR=$(VSIX_STAGE_DIR) DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) PACKAGE_NAME=$(PACKAGE_NAME) PACKAGE_VERSION=$(PACKAGE_VERSION) PACKAGE_RELEASE=$(PACKAGE_RELEASE) LIBC_PROFILE=$(LIBC_PROFILE) LIBC_FLOAT=$(LIBC_FLOAT) LIBC_DOUBLE=$(LIBC_DOUBLE) LIBC_LONG=$(LIBC_LONG) LIBC_LONGLONG=$(LIBC_LONGLONG) LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT) X_MODEL=$(X_MODEL)

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
		LIBC_LONGLONG=$(LIBC_LONGLONG) \
		LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT) \
		X_MODEL=$(X_MODEL)

x-models: x-s x-m x-l

x-s:
	@$(MAKE) -C $(X_ROOT) x \
		REPO_ROOT=$(ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)/x-s \
		DIST_DIR=$(OUT_DIR)/x-s \
		OUT_DIR=$(OUT_DIR)/x-s-out \
		Z_DIST_DIR=$(OUT_DIR)/x-s-z \
		HOST_BIN_DIR=$(OUT_DIR)/x-s/bin \
		PUBLIC_INCLUDE_DIR=$(OUT_DIR)/x-s/include \
		PUBLIC_LIB_DIR=$(OUT_DIR)/x-s/lib \
		TARGET_DIR=$(OUT_DIR)/x-s/z80 \
		TARGET_INCLUDE_DIR=$(OUT_DIR)/x-s/z80/include \
		TARGET_LIB_DIR=$(OUT_DIR)/x-s/z80/lib \
		SHARE_DIR=$(OUT_DIR)/x-s/share \
		DOCS_DIR=$(OUT_DIR)/x-s/share/doc \
		PKG_DIR=$(OUT_DIR)/x-s/pkg \
		VSIX_STAGE_DIR=$(OUT_DIR)/x-s/pkg/vsix \
		PLATFORM=$(PLATFORM) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) \
		ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) \
		LIBC_PROFILE=$(LIBC_PROFILE) \
		LIBC_FLOAT=0 \
		LIBC_DOUBLE=0 \
		LIBC_LONG=0 \
		LIBC_LONGLONG=0 \
		LIBC_STDIO_FLOAT=0 \
		X_MODEL=S \
		PACKAGE_NAME=x-s

x-m:
	@$(MAKE) -C $(X_ROOT) x \
		REPO_ROOT=$(ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)/x-m \
		DIST_DIR=$(OUT_DIR)/x-m \
		OUT_DIR=$(OUT_DIR)/x-m-out \
		Z_DIST_DIR=$(OUT_DIR)/x-m-z \
		HOST_BIN_DIR=$(OUT_DIR)/x-m/bin \
		PUBLIC_INCLUDE_DIR=$(OUT_DIR)/x-m/include \
		PUBLIC_LIB_DIR=$(OUT_DIR)/x-m/lib \
		TARGET_DIR=$(OUT_DIR)/x-m/z80 \
		TARGET_INCLUDE_DIR=$(OUT_DIR)/x-m/z80/include \
		TARGET_LIB_DIR=$(OUT_DIR)/x-m/z80/lib \
		SHARE_DIR=$(OUT_DIR)/x-m/share \
		DOCS_DIR=$(OUT_DIR)/x-m/share/doc \
		PKG_DIR=$(OUT_DIR)/x-m/pkg \
		VSIX_STAGE_DIR=$(OUT_DIR)/x-m/pkg/vsix \
		PLATFORM=$(PLATFORM) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) \
		ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) \
		LIBC_PROFILE=$(LIBC_PROFILE) \
		LIBC_FLOAT=1 \
		LIBC_DOUBLE=0 \
		LIBC_LONG=1 \
		LIBC_LONGLONG=0 \
		LIBC_STDIO_FLOAT=0 \
		X_MODEL=M \
		PACKAGE_NAME=x-m

x-l:
	@$(MAKE) -C $(X_ROOT) x \
		REPO_ROOT=$(ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)/x-l \
		DIST_DIR=$(OUT_DIR)/x-l \
		OUT_DIR=$(OUT_DIR)/x-l-out \
		Z_DIST_DIR=$(OUT_DIR)/x-l-z \
		HOST_BIN_DIR=$(OUT_DIR)/x-l/bin \
		PUBLIC_INCLUDE_DIR=$(OUT_DIR)/x-l/include \
		PUBLIC_LIB_DIR=$(OUT_DIR)/x-l/lib \
		TARGET_DIR=$(OUT_DIR)/x-l/z80 \
		TARGET_INCLUDE_DIR=$(OUT_DIR)/x-l/z80/include \
		TARGET_LIB_DIR=$(OUT_DIR)/x-l/z80/lib \
		SHARE_DIR=$(OUT_DIR)/x-l/share \
		DOCS_DIR=$(OUT_DIR)/x-l/share/doc \
		PKG_DIR=$(OUT_DIR)/x-l/pkg \
		VSIX_STAGE_DIR=$(OUT_DIR)/x-l/pkg/vsix \
		PLATFORM=$(PLATFORM) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) \
		ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) \
		LIBC_PROFILE=$(LIBC_PROFILE) \
		LIBC_FLOAT=1 \
		LIBC_DOUBLE=1 \
		LIBC_LONG=1 \
		LIBC_LONGLONG=1 \
		LIBC_STDIO_FLOAT=1 \
		X_MODEL=L \
		PACKAGE_NAME=x-l

test-x-s: x-s
	bash x/tests/run_tests.sh $(OUT_DIR)/x-s/bin/xcc --filter model-s

test-x-m: x-m
	bash x/tests/run_tests.sh $(OUT_DIR)/x-m/bin/xcc --filter model-m

test-x-l: x-l
	bash x/tests/run_tests.sh $(OUT_DIR)/x-l/bin/xcc --filter model-l

test-x-models: test-x-s test-x-m test-x-l

stage-includes:
	@$(MAKE) -C $(X_ROOT) stage-includes REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR)

stage-target-assets:
	@$(MAKE) -C $(X_ROOT) stage-target-assets REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR)

stage-xcc-support:
	@$(MAKE) -C $(X_ROOT) stage-xcc-support REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) OUT_DIR=$(OUT_DIR) Z_DIST_DIR=$(Z_DIST_DIR) PLATFORM=$(PLATFORM) DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) ZX_RAM_STORAGE=$(ZX_RAM_STORAGE) LIBC_PROFILE=$(LIBC_PROFILE) LIBC_FLOAT=$(LIBC_FLOAT) LIBC_DOUBLE=$(LIBC_DOUBLE) LIBC_LONG=$(LIBC_LONG) LIBC_LONGLONG=$(LIBC_LONGLONG) LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT) X_MODEL=$(X_MODEL)

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
		'  all                  Build x tools and y outputs (default).' \
		'  packages             Build the optional x packaging artifacts.' \
		'  x                    Build only the x compiler suite distribution.' \
		'  x-models             Build the S, M, and L x distributions.' \
		'  x-s                  Build the S model into bin/x-s.' \
		'  x-m                  Build the M model into bin/x-m.' \
		'  x-l                  Build the L model into bin/x-l.' \
		'  test-x-s             Build x-s and run the model-s compatible suite.' \
		'  test-x-m             Build x-m and run the model-m compatible suite.' \
		'  test-x-l             Build x-l and run the model-l compatible suite.' \
		'  test-x-models        Build all x models and run all compatible suites.' \
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
		'  X_MODEL=S|M|L                   Release model defaults (default: L).' \
		'  LIBC_PROFILE=name               Libc build flavor label (default: full).' \
		'  LIBC_FLOAT=0|1                  Enable float libc/runtime support.' \
		'  LIBC_DOUBLE=0|1                 Enable double libc/runtime support.' \
		'  LIBC_LONG=0|1                   Enable long libc/runtime support.' \
		'  LIBC_LONGLONG=0|1               Enable long long libc/runtime support.' \
		'  LIBC_STDIO_FLOAT=0|1            Enable float scanf/printf conversions.' \
		'  PACKAGE_NAME=name               Package name (default: x).' \
		'  PACKAGE_VERSION=x.y.z           Package version (default: latest vX.Y.Z tag).' \
		'  PACKAGE_RELEASE=n               Package release number (default: 1).'

clean:
	rm -rf $(OUT_DIR) $(BUILD_DIR)
	@$(MAKE) -C $(X_ROOT)/pkg clean REPO_ROOT=$(ROOT) X_ROOT=$(X_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(X_DIST_DIR) VSIX_STAGE_DIR=$(VSIX_STAGE_DIR)
