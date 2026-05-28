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
export DOCS_DIR           := $(DIST_DIR)/docs
export PUBLIC_INCLUDE_DIR := $(DIST_DIR)/include
export PUBLIC_LIB_DIR     := $(DIST_DIR)/lib
export VSCODE_EXT_DIR     := $(DIST_DIR)/extensions/vscode
export ZX_TARGET_DIR      := $(DIST_DIR)/targets/zxspectrum
export ZX_ROMS_DIR        := $(ZX_TARGET_DIR)/roms
export ZX_APPS_DIR        := $(ZX_TARGET_DIR)/apps
export ZX_MDR_DIR         := $(ZX_TARGET_DIR)/mdr

# Compilable sub-projects — order matters (dependencies first).
# Add new entries here as sub-projects become ready; do not add make targets.
SUBDIRS := lib tools src/xc src/yos

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
	@echo "==> dist docs"
	@$(MAKE) stage-dist-docs
	@echo "==> packages"
	@$(MAKE) -C pkg all

.PHONY: stage-includes
stage-includes:
	rm -rf $(PUBLIC_INCLUDE_DIR)
	mkdir -p $(PUBLIC_INCLUDE_DIR)
	cp -R $(ROOT)/include/. $(PUBLIC_INCLUDE_DIR)/

.PHONY: stage-target-assets
stage-target-assets:
	mkdir -p $(ZX_MDR_DIR)
	cp $(ROOT)/tests/microdrives/hello.mdr $(ZX_MDR_DIR)/

.PHONY: stage-dist-docs
stage-dist-docs:
	mkdir -p $(DIST_DIR)
	rm -rf $(DIST_DIR)/doc $(DOCS_DIR)
	mkdir -p $(DOCS_DIR)
	cp $(ROOT)/docs/dist/README.md $(DIST_DIR)/README.md
	cp $(ROOT)/tools/appmake/README.md $(DOCS_DIR)/APPMAKE.md
	cp $(ROOT)/tools/microdrive/README.md $(DOCS_DIR)/MICRODRIVE.md
	cp $(ROOT)/tools/serial/README.md $(DOCS_DIR)/SERIAL.md
	cp $(ROOT)/src/xc/xlink/README.md $(DOCS_DIR)/XLINK.md
	cp $(ROOT)/src/yos/README.md $(DOCS_DIR)/YOS.md
	cp $(ROOT)/docs/DEBUGGER_INTEGRATION.md $(DOCS_DIR)/DEBUGGER_INTEGRATION.md

.PHONY: clean
clean:
	rm -rf $(DIST_DIR) $(BUILD_DIR)
	@$(MAKE) -C pkg clean
