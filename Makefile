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
export DIST_DIR           := $(ROOT)/bin
export HOST_BIN_DIR       := $(DIST_DIR)/bin
export PUBLIC_INCLUDE_DIR := $(DIST_DIR)/include
export TARGET_INCLUDE_DIR := $(PUBLIC_INCLUDE_DIR)/z80
export TARGET_PLATFORM_INCLUDE_DIR := $(TARGET_INCLUDE_DIR)/platform
export PUBLIC_LIB_DIR     := $(DIST_DIR)/lib
export TARGET_LIB_DIR     := $(PUBLIC_LIB_DIR)/z80
export ZX_PLATFORM_LIB_DIR := $(TARGET_LIB_DIR)/spectrum
export Z80_DIST_DIR       := $(DIST_DIR)/z80
export Z80_BIN_DIR        := $(Z80_DIST_DIR)/bin
export ZX_TARGET_DIR      := $(Z80_DIST_DIR)/spectrum
export ZX_TARGET_BIN_DIR  := $(ZX_TARGET_DIR)/bin
export ZX_TARGET_INCLUDE_DIR := $(ZX_TARGET_DIR)/include
export ZX_TARGET_LIB_OUT_DIR := $(ZX_TARGET_DIR)/lib
export ZX_ROMS_DIR        := $(ZX_TARGET_BIN_DIR)
export ZX_APPS_DIR        := $(ZX_TARGET_BIN_DIR)/apps
export ZX_MDR_DIR         := $(ZX_TARGET_BIN_DIR)/mdr
export LIBEXEC_DIR        := $(DIST_DIR)/libexec/xcc
export XCC_SUPPORT_INCLUDE_DIR := $(LIBEXEC_DIR)/include
export XCC_SUPPORT_RUNTIME_DIR := $(LIBEXEC_DIR)/runtime
export SHARE_DIR          := $(DIST_DIR)/share/xtools
export DOCS_DIR           := $(SHARE_DIR)/docs
export EXAMPLES_DIR       := $(SHARE_DIR)/examples
export TEMPLATES_DIR      := $(SHARE_DIR)/templates
export PKG_DIR            := $(DIST_DIR)/pkg
export VSIX_STAGE_DIR     := $(PKG_DIR)/vsix

# Compilable sub-projects — order matters (dependencies first).
# Add new entries here as sub-projects become ready; do not add make targets.
SUBDIRS := lib tools src/xc lib/libc src/yos

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
	@echo "==> xcc support"
	@$(MAKE) stage-xcc-support
	@echo "==> dist docs"
	@$(MAKE) stage-dist-docs
	@echo "==> packages"
	@$(MAKE) -C pkg all

.PHONY: stage-includes
stage-includes:
	rm -rf $(PUBLIC_INCLUDE_DIR)
	mkdir -p $(PUBLIC_INCLUDE_DIR) $(TARGET_INCLUDE_DIR) $(TARGET_PLATFORM_INCLUDE_DIR)
	cp -R $(ROOT)/include/. $(PUBLIC_INCLUDE_DIR)/
	rm -f $(PUBLIC_INCLUDE_DIR)/yos.h
	cp -R $(ROOT)/lib/libc/include/. $(TARGET_INCLUDE_DIR)/
	cp $(ROOT)/include/yos.h $(TARGET_PLATFORM_INCLUDE_DIR)/
	cp -R $(ROOT)/lib/xbfd/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(ROOT)/lib/rsp/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(ROOT)/lib/xgdb/include/. $(PUBLIC_INCLUDE_DIR)/

.PHONY: stage-target-assets
stage-target-assets:
	rm -rf $(DIST_DIR)/targets
	mkdir -p $(TARGET_LIB_DIR) $(ZX_PLATFORM_LIB_DIR)
	mkdir -p $(Z80_BIN_DIR)
	mkdir -p $(ZX_TARGET_BIN_DIR) $(ZX_TARGET_INCLUDE_DIR) $(ZX_TARGET_LIB_OUT_DIR)
	mkdir -p $(ZX_APPS_DIR) $(ZX_MDR_DIR)
	mkdir -p $(PKG_DIR)/deb $(PKG_DIR)/vsix
	cp $(ROOT)/tests/microdrives/hello.mdr $(ZX_MDR_DIR)/

.PHONY: stage-xcc-support
stage-xcc-support:
	rm -rf $(LIBEXEC_DIR)
	mkdir -p $(XCC_SUPPORT_INCLUDE_DIR) $(XCC_SUPPORT_RUNTIME_DIR)
	cp -R $(ROOT)/lib/libc/include/. $(XCC_SUPPORT_INCLUDE_DIR)/
	@for src in $(sort $(wildcard $(ROOT)/src/xc/xcc/lib/runtime/*.s)); do \
	    rel="$(XCC_SUPPORT_RUNTIME_DIR)/$$(basename "$${src%.s}").rel"; \
	    $(HOST_BIN_DIR)/xas --mode=sdcc "$$src" -o "$$rel"; \
	done
	@rm -f $(XCC_SUPPORT_RUNTIME_DIR)/z80.lib $(XCC_SUPPORT_RUNTIME_DIR)/runtime.lib
	$(HOST_BIN_DIR)/xar rcs $(XCC_SUPPORT_RUNTIME_DIR)/z80.lib $(XCC_SUPPORT_RUNTIME_DIR)/*.rel
	cp $(XCC_SUPPORT_RUNTIME_DIR)/z80.lib $(XCC_SUPPORT_RUNTIME_DIR)/runtime.lib

.PHONY: stage-dist-docs
stage-dist-docs:
	mkdir -p $(DIST_DIR)
	rm -rf $(DIST_DIR)/doc $(DIST_DIR)/docs $(DIST_DIR)/extensions $(DOCS_DIR)
	mkdir -p $(DOCS_DIR) $(EXAMPLES_DIR) $(TEMPLATES_DIR) $(PKG_DIR)/deb $(PKG_DIR)/vsix
	cp $(ROOT)/docs/dist/README.md $(DIST_DIR)/README.md
	cp $(ROOT)/tools/appmake/README.md $(DOCS_DIR)/APPMAKE.md
	cp $(ROOT)/tools/microdrive/README.md $(DOCS_DIR)/MICRODRIVE.md
	cp $(ROOT)/tools/serial/README.md $(DOCS_DIR)/SERIAL.md
	cp $(ROOT)/src/xc/xld/README.md $(DOCS_DIR)/XLD.md
	cp $(ROOT)/src/yos/README.md $(DOCS_DIR)/YOS.md
	cp $(ROOT)/docs/DEBUGGER_INTEGRATION.md $(DOCS_DIR)/DEBUGGER_INTEGRATION.md

.PHONY: clean
clean:
	rm -rf $(DIST_DIR) $(BUILD_DIR)
	@$(MAKE) -C pkg clean
