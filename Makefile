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

# If HEAD is tagged as vX.Y.Z, use X.Y.Z as the default package version.
GIT_EXACT_TAG := $(shell git describe --tags --exact-match 2>/dev/null || true)

# Repository root and shared output directories (exported for sub-makes).
export PACKAGE_NAME       ?= xtools
ifneq ($(filter v%,$(GIT_EXACT_TAG)),)
export PACKAGE_VERSION    ?= $(patsubst v%,%,$(GIT_EXACT_TAG))
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
export PLATFORM           ?= cpm3
export DEFAULT_PLATFORM   := $(PLATFORM)
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

# Compilable sub-projects — order matters (dependencies first).
# Add new entries here as sub-projects become ready; do not add make targets.
SUBDIRS := lib tools src/xc lib/libc
YOS_SUBDIR := src/yos
XTOOLS_SUBDIRS := lib tools src/xc/xld src/xc/xgdb src/xc/xcc src/xc/xas src/xc/xar src/xc/xobjcopy lib/libc
RUNTIME_ARCHIVE_NAME := libruntime.a
PLATFORM_ARCHIVE_NAME := lib$(PLATFORM).a
PLATFORM_SYS_DIR := $(ROOT)/lib/sys/$(PLATFORM)
TOOLCHAIN_RUNTIME_BUILD_DIR := $(BUILD_DIR)/toolchain-runtime
TOOLCHAIN_PLATFORM_BUILD_DIR := $(BUILD_DIR)/toolchain-platform/$(PLATFORM)

.PHONY: all
all:
	@for d in $(SUBDIRS); do \
	    echo "==> $$d"; \
	    $(MAKE) -C $$d || exit 1; \
	done
	@echo "==> include"
	@$(MAKE) stage-includes
	@echo "==> target assets"
	@$(MAKE) stage-target-assets
	@echo "==> layout cleanup"
	@$(MAKE) stage-layout-cleanup
	@echo "==> xcc support"
	@$(MAKE) stage-xcc-support
	@echo "==> dist docs"
	@$(MAKE) stage-dist-docs
	@echo "==> $(YOS_SUBDIR)"
	@$(MAKE) -C $(YOS_SUBDIR) DIST_DIR=$(Y_DIST_DIR) ZX_ROMS_DIR=$(ZX_ROMS_DIR)
	@echo "==> packages"
	@$(MAKE) -C pkg all

.PHONY: xtools
xtools:
	@for d in $(XTOOLS_SUBDIRS); do \
	    echo "==> $$d"; \
	    $(MAKE) -C $$d || exit 1; \
	done
	@echo "==> include"
	@$(MAKE) stage-includes
	@echo "==> target assets"
	@$(MAKE) stage-target-assets
	@echo "==> layout cleanup"
	@$(MAKE) stage-layout-cleanup
	@echo "==> xcc support"
	@$(MAKE) stage-xcc-support
	@echo "==> dist docs"
	@$(MAKE) stage-dist-docs

.PHONY: stage-includes
stage-includes:
	rm -rf $(PUBLIC_INCLUDE_DIR) $(TARGET_INCLUDE_DIR)
	mkdir -p $(PUBLIC_INCLUDE_DIR) $(TARGET_INCLUDE_DIR)
	cp -R $(ROOT)/lib/xbfd/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(ROOT)/lib/rsp/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(ROOT)/lib/xgdb/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(ROOT)/lib/libc/include/. $(TARGET_INCLUDE_DIR)/
	cp $(ROOT)/include/yos.h $(TARGET_INCLUDE_DIR)/yos.h

.PHONY: stage-target-assets
stage-target-assets:
	rm -rf $(Z80_DIST_DIR)
	mkdir -p $(Z80_BIN_DIR)
	mkdir -p $(ZX_TARGET_BIN_DIR) $(ZX_TARGET_INCLUDE_DIR) $(ZX_TARGET_LIB_OUT_DIR)
	mkdir -p $(ZX_APPS_DIR) $(ZX_MDR_DIR)
	mkdir -p $(PKG_DIR)/deb $(PKG_DIR)/vsix
	cp $(ROOT)/tests/microdrives/hello.mdr $(ZX_MDR_DIR)/

.PHONY: stage-xcc-support
stage-xcc-support:
	rm -rf $(TOOLCHAIN_RUNTIME_BUILD_DIR) $(TOOLCHAIN_PLATFORM_BUILD_DIR)
	rm -f $(TARGET_LIB_DIR)/crt0.rel $(TARGET_LIB_DIR)/crt0.s \
	      $(TARGET_LIB_DIR)/linker.lk $(TARGET_LIB_DIR)/linker.ld \
	      $(TARGET_LIB_DIR)/$(RUNTIME_ARCHIVE_NAME) \
	      $(TARGET_LIB_DIR)/$(PLATFORM_ARCHIVE_NAME)
	mkdir -p $(TARGET_LIB_DIR) $(TOOLCHAIN_RUNTIME_BUILD_DIR) $(TOOLCHAIN_PLATFORM_BUILD_DIR)
	@for src in $$(find $(ROOT)/src/xc/xcc/lib/runtime -name '*.s' | sort); do \
	    rel="$(TOOLCHAIN_RUNTIME_BUILD_DIR)/$$(basename "$${src%.s}").rel"; \
	    $(HOST_BIN_DIR)/xas --mode=sdcc "$$src" -o "$$rel"; \
	done
	$(HOST_BIN_DIR)/xar --mode=gnu rcs $(TARGET_LIB_DIR)/$(RUNTIME_ARCHIVE_NAME) $(TOOLCHAIN_RUNTIME_BUILD_DIR)/*.rel
	$(HOST_BIN_DIR)/xas --mode=sdcc $(PLATFORM_SYS_DIR)/crt0.s -o $(TARGET_LIB_DIR)/crt0.rel
	cp $(PLATFORM_SYS_DIR)/crt0.s $(TARGET_LIB_DIR)/crt0.s
	cp $(PLATFORM_SYS_DIR)/linker.lk $(TARGET_LIB_DIR)/
	cp $(PLATFORM_SYS_DIR)/linker.ld $(TARGET_LIB_DIR)/
	@for src in $$(find $(PLATFORM_SYS_DIR) -maxdepth 1 \( -name '*.s' -o -name '*.c' \) ! -name 'crt0.s' | sort); do \
	    stem="$$(basename "$${src%.*}")"; \
	    rel="$(TOOLCHAIN_PLATFORM_BUILD_DIR)/$$stem.rel"; \
	    case "$$src" in \
	        *.c) asm="$(TOOLCHAIN_PLATFORM_BUILD_DIR)/$$stem.s"; \
	             $(HOST_BIN_DIR)/xcc -S -I$(ROOT)/lib/libc/include -I$(ROOT)/include -o "$$asm" "$$src"; \
	             $(HOST_BIN_DIR)/xas --mode=sdcc "$$asm" -o "$$rel" ;; \
	        *.s) $(HOST_BIN_DIR)/xas --mode=sdcc "$$src" -o "$$rel" ;; \
	    esac; \
	done
	$(HOST_BIN_DIR)/xar --mode=gnu rcs $(TARGET_LIB_DIR)/$(PLATFORM_ARCHIVE_NAME) $(TOOLCHAIN_PLATFORM_BUILD_DIR)/*.rel

# Remove leftovers from the pre-z80/ mixed layout so an incremental build
# converges to the clean prefix.
.PHONY: stage-layout-cleanup
stage-layout-cleanup:
	rm -f $(X_DIST_DIR)/bin/appmake \
	      $(X_DIST_DIR)/bin/microdrive \
	      $(X_DIST_DIR)/bin/serial \
	      $(X_DIST_DIR)/lib/libmicrodrive.a \
	      $(X_DIST_DIR)/lib/libc.a \
	      $(X_DIST_DIR)/lib/libruntime.a \
	      $(X_DIST_DIR)/lib/lib$(PLATFORM).a \
	      $(X_DIST_DIR)/lib/crt0.rel \
	      $(X_DIST_DIR)/lib/crt0.s \
	      $(X_DIST_DIR)/lib/linker.ld \
	      $(X_DIST_DIR)/lib/linker.lk

.PHONY: stage-toolchain-targets
stage-toolchain-targets:
	@:

.PHONY: stage-toolchain-prefixes
stage-toolchain-prefixes:
	@:

# share/ carries only the compiler tool manuals.
.PHONY: stage-dist-docs
stage-dist-docs:
	mkdir -p $(DIST_DIR)
	rm -rf $(DIST_DIR)/doc $(DIST_DIR)/docs $(DIST_DIR)/extensions $(DIST_DIR)/share
	mkdir -p $(DOCS_DIR) $(PKG_DIR)/deb $(PKG_DIR)/vsix
	cp $(ROOT)/docs/dist/README.md $(DIST_DIR)/README.md
	cp $(ROOT)/docs/dist/man/*.md $(DOCS_DIR)/

.PHONY: clean
clean:
	rm -rf $(OUT_DIR) $(BUILD_DIR)
	@$(MAKE) -C pkg clean
