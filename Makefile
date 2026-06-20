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
TARGET_C8FLAGS := -I$(ROOT)/lib/libc/include -I$(ROOT)/include $(TARGET_FEATURE_DEFINES)
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

# Compilable sub-projects — order matters (dependencies first).
# Add new entries here as sub-projects become ready; do not add make targets.
SUBDIRS := lib tools src/xc lib/libc
YOS_SUBDIR := src/yos
XTOOLS_SUBDIRS := lib tools src/xc/xld src/xc/xgdb src/xc/xcc src/xc/xopt src/xc/xas src/xc/xar src/xc/xobjcopy lib/libc
RUNTIME_ARCHIVE_NAME := libruntime.a
FIXED_ARCHIVE_NAME := libfixed.a
STAGED_PLATFORMS := $(sort $(PLATFORM) cpm3 emu)
PLATFORM_ARCHIVE_NAME := lib$(PLATFORM).a
PLATFORM_SYS_DIR := $(ROOT)/lib/sys/$(PLATFORM)
TOOLCHAIN_RUNTIME_BUILD_DIR := $(BUILD_DIR)/toolchain-runtime
TOOLCHAIN_FIXED_BUILD_DIR := $(BUILD_DIR)/toolchain-fixed
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
	cp -R $(ROOT)/lib/xopt/include/. $(PUBLIC_INCLUDE_DIR)/
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
	rm -rf $(TOOLCHAIN_RUNTIME_BUILD_DIR) $(TOOLCHAIN_FIXED_BUILD_DIR) $(BUILD_DIR)/toolchain-platform
	rm -f $(TARGET_LIB_DIR)/crt0.rel $(TARGET_LIB_DIR)/crt0.s \
	      $(TARGET_LIB_DIR)/linker.lk $(TARGET_LIB_DIR)/linker.ld \
	      $(TARGET_LIB_DIR)/$(RUNTIME_ARCHIVE_NAME) \
	      $(TARGET_LIB_DIR)/$(FIXED_ARCHIVE_NAME) \
	      $(TARGET_LIB_DIR)/crt0-*.rel $(TARGET_LIB_DIR)/crt0-*.s \
	      $(TARGET_LIB_DIR)/linker-*.lk $(TARGET_LIB_DIR)/linker-*.ld
	@for platform in $(STAGED_PLATFORMS); do \
	    rm -f "$(TARGET_LIB_DIR)/lib$$platform.a"; \
	done
	mkdir -p $(TARGET_LIB_DIR) $(TOOLCHAIN_RUNTIME_BUILD_DIR) $(TOOLCHAIN_FIXED_BUILD_DIR) $(TOOLCHAIN_PLATFORM_BUILD_DIR)
	@for src in $$(find $(ROOT)/src/xc/xcc/lib/runtime -name '*.s' \
	        ! -path '*/8_8/*' \
	        ! -path '*/16_16/*' \
	        ! -path '*/24_8/*' | sort); do \
	    rel="$(TOOLCHAIN_RUNTIME_BUILD_DIR)/$$(basename "$${src%.s}").rel"; \
	    base="$$(basename "$${src%.s}")"; \
	    skip=0; \
	    case "$$base" in \
	        fs*|*2fs|fs2*|fp*|fitosf|fstoi|cabsf|cargf|cimag|conjf|creal|complex_i) \
	            if [ "$(LIBC_FLOAT)" = "0" ]; then skip=1; fi ;; \
	    esac; \
	    case "$$base" in \
	        db*|*2db|db2*) \
	            if [ "$(LIBC_DOUBLE)" = "0" ]; then skip=1; fi ;; \
	    esac; \
	    case "$$base" in \
	        *long*) \
	            if [ "$(LIBC_LONG)" = "0" ]; then skip=1; fi ;; \
	    esac; \
	    case "$$base" in \
	        db2ll|divll|divsll|divull|ll2*|modll|modsll|modull|mulll|shl64|shr64s|shr64u|sint2ll|slong2ll|uint2ll|ull2db|ulong2ll) \
	            if [ "$(LIBC_LONGLONG)" = "0" ]; then skip=1; fi ;; \
	    esac; \
	    if [ "$$skip" = "1" ]; then continue; fi; \
	    $(HOST_BIN_DIR)/xas $(TARGET_AS8FLAGS) "$$src" -o "$$rel"; \
	done
	$(HOST_BIN_DIR)/xar --mode=gnu rcs $(TARGET_LIB_DIR)/$(RUNTIME_ARCHIVE_NAME) $(TOOLCHAIN_RUNTIME_BUILD_DIR)/*.rel
	@for src in $$(find \
	        $(ROOT)/src/xc/xcc/lib/runtime/8_8 \
	        $(ROOT)/src/xc/xcc/lib/runtime/16_16 \
	        $(ROOT)/src/xc/xcc/lib/runtime/24_8 \
	        -name '*.s' | sort); do \
	    stem="$$(basename "$${src%.*}")"; \
	    opt="$(TOOLCHAIN_FIXED_BUILD_DIR)/$$stem.opt.s"; \
	    rel="$(TOOLCHAIN_FIXED_BUILD_DIR)/$$stem.rel"; \
	    $(HOST_BIN_DIR)/xopt -O3 "$$src" -o "$$opt"; \
	    $(HOST_BIN_DIR)/xas $(TARGET_AS8FLAGS) "$$opt" -o "$$rel"; \
	done
	$(HOST_BIN_DIR)/xar --mode=gnu rcs $(TARGET_LIB_DIR)/$(FIXED_ARCHIVE_NAME) $(TOOLCHAIN_FIXED_BUILD_DIR)/*.rel
	@for platform in $(STAGED_PLATFORMS); do \
	    sysdir="$(ROOT)/lib/sys/$$platform"; \
	    builddir="$(BUILD_DIR)/toolchain-platform/$$platform"; \
	    mkdir -p "$$builddir"; \
	    $(HOST_BIN_DIR)/xas $(TARGET_AS8FLAGS) "$$sysdir/crt0.s" -o "$(TARGET_LIB_DIR)/crt0-$$platform.rel"; \
	    cp "$$sysdir/crt0.s" "$(TARGET_LIB_DIR)/crt0-$$platform.s"; \
	    cp "$$sysdir/linker.lk" "$(TARGET_LIB_DIR)/linker-$$platform.lk"; \
	    cp "$$sysdir/linker.ld" "$(TARGET_LIB_DIR)/linker-$$platform.ld"; \
	    for src in $$(find "$$sysdir" -maxdepth 1 \( -name '*.s' -o -name '*.c' \) ! -name 'crt0.s' | sort); do \
	        stem="$$(basename "$${src%.*}")"; \
	        rel="$$builddir/$$stem.rel"; \
	        case "$$src" in \
	            *.c) asm="$$builddir/$$stem.s"; \
	                 $(HOST_BIN_DIR)/xcc -S $(TARGET_C8FLAGS) -o "$$asm" "$$src"; \
	                 $(HOST_BIN_DIR)/xas $(TARGET_AS8FLAGS) "$$asm" -o "$$rel" ;; \
	            *.s) $(HOST_BIN_DIR)/xas $(TARGET_AS8FLAGS) "$$src" -o "$$rel" ;; \
	        esac; \
	    done; \
	    $(HOST_BIN_DIR)/xar --mode=gnu rcs "$(TARGET_LIB_DIR)/lib$$platform.a" "$$builddir"/*.rel; \
	done
	$(MAKE) -C $(ROOT)/lib/libc \
	    C8FLAGS="$(TARGET_C8FLAGS)" \
	    AS8FLAGS="$(TARGET_AS8FLAGS)" \
	    LIBC_PROFILE="$(LIBC_PROFILE)" \
	    LIBC_FLOAT="$(LIBC_FLOAT)" \
	    LIBC_DOUBLE="$(LIBC_DOUBLE)" \
	    LIBC_LONG="$(LIBC_LONG)" \
	    LIBC_LONGLONG="$(LIBC_LONGLONG)"
	cp $(TARGET_LIB_DIR)/crt0-$(PLATFORM).rel $(TARGET_LIB_DIR)/crt0.rel
	cp $(TARGET_LIB_DIR)/crt0-$(PLATFORM).s $(TARGET_LIB_DIR)/crt0.s
	cp $(TARGET_LIB_DIR)/linker-$(PLATFORM).lk $(TARGET_LIB_DIR)/linker.lk
	cp $(TARGET_LIB_DIR)/linker-$(PLATFORM).ld $(TARGET_LIB_DIR)/linker.ld

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
