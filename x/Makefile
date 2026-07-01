REPO_ROOT ?= $(abspath ..)
X_ROOT := $(abspath .)
Y_ROOT ?= $(REPO_ROOT)/y
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/x
OUT_DIR ?= $(REPO_ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_INCLUDE_DIR ?= $(DIST_DIR)/include
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
TARGET_DIR ?= $(DIST_DIR)/z80
TARGET_INCLUDE_DIR ?= $(TARGET_DIR)/include
TARGET_LIB_DIR ?= $(TARGET_DIR)/lib
Z_DIST_DIR ?= $(OUT_DIR)/z
Z80_DIST_DIR ?= $(Z_DIST_DIR)/z80
Z80_BIN_DIR ?= $(Z80_DIST_DIR)/bin
ZX_TARGET_DIR ?= $(Z80_DIST_DIR)/spectrum
ZX_TARGET_BIN_DIR ?= $(ZX_TARGET_DIR)/bin
ZX_TARGET_INCLUDE_DIR ?= $(ZX_TARGET_DIR)/include
ZX_TARGET_LIB_OUT_DIR ?= $(ZX_TARGET_DIR)/lib
ZX_APPS_DIR ?= $(ZX_TARGET_BIN_DIR)/apps
ZX_MDR_DIR ?= $(ZX_TARGET_BIN_DIR)/mdr
SHARE_DIR ?= $(DIST_DIR)/share
DOCS_DIR ?= $(SHARE_DIR)/doc
PKG_DIR ?= $(DIST_DIR)/pkg
VSIX_STAGE_DIR ?= $(PKG_DIR)/vsix
PLATFORM ?= none
DEFAULT_PLATFORM ?= $(PLATFORM)
ZX_RAM_STORAGE ?= plus3
LIBC_PROFILE ?= full
include $(X_ROOT)/mk/model.mk

LIBC_FLOAT ?= $(X_MODEL_LIBC_FLOAT_DEFAULT)
LIBC_DOUBLE ?= $(X_MODEL_LIBC_DOUBLE_DEFAULT)
LIBC_LONG ?= $(X_MODEL_LIBC_LONG_DEFAULT)
LIBC_LONGLONG ?= $(X_MODEL_LIBC_LONGLONG_DEFAULT)
LIBC_STDIO_FLOAT ?= $(X_MODEL_LIBC_STDIO_FLOAT_DEFAULT)
ifeq ($(LIBC_FLOAT),0)
override LIBC_STDIO_FLOAT := 0
endif

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

RUNTIME_ARCHIVE_NAME := libruntime.a
FIXED_ARCHIVE_NAME := libfixed.a
STAGED_PLATFORMS := $(sort $(PLATFORM) cpm3 emu)
TOOLCHAIN_RUNTIME_BUILD_DIR := $(BUILD_DIR)/toolchain-runtime
TOOLCHAIN_FIXED_BUILD_DIR := $(BUILD_DIR)/toolchain-fixed
WINDOWS_MINGW_DOCKER_SCRIPT := $(X_ROOT)/tests/tools/windows_host_preflight_docker.sh

.PHONY: all x clean
.PHONY: stage-includes stage-target-assets stage-xcc-support
.PHONY: stage-layout-cleanup stage-dist-docs
.PHONY: windows-host-preflight

all: x

windows-host-preflight:
	bash $(WINDOWS_MINGW_DOCKER_SCRIPT)

x:
	@$(MAKE) -C lib \
		REPO_ROOT=$(REPO_ROOT) \
		X_ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	@$(MAKE) -C src \
		REPO_ROOT=$(REPO_ROOT) \
		X_ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM)
	@$(MAKE) -C libc \
		ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) \
		LIBC_PROFILE=$(LIBC_PROFILE) \
		LIBC_FLOAT=$(LIBC_FLOAT) \
		LIBC_DOUBLE=$(LIBC_DOUBLE) \
		LIBC_LONG=$(LIBC_LONG) \
		LIBC_LONGLONG=$(LIBC_LONGLONG) \
		LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT) \
		X_MODEL=$(X_MODEL_NAME) \
		C8FLAGS="$(TARGET_C8FLAGS)" \
		AS8FLAGS="$(TARGET_AS8FLAGS)"
	@$(MAKE) stage-includes
	@$(MAKE) stage-target-assets
	@$(MAKE) stage-layout-cleanup
	@$(MAKE) stage-xcc-support
	@$(MAKE) stage-dist-docs

stage-includes:
	rm -rf $(PUBLIC_INCLUDE_DIR) $(TARGET_INCLUDE_DIR)
	mkdir -p $(PUBLIC_INCLUDE_DIR) $(TARGET_INCLUDE_DIR)
	cp -R $(X_ROOT)/lib/xbfd/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(X_ROOT)/lib/xopt/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(X_ROOT)/lib/rsp/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(X_ROOT)/lib/xz80/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(X_ROOT)/lib/xgdb/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(X_ROOT)/lib/xemu/include/. $(PUBLIC_INCLUDE_DIR)/
	cp -R $(X_ROOT)/libc/include/. $(TARGET_INCLUDE_DIR)/
	cp -R $(X_ROOT)/platforms/include/. $(TARGET_INCLUDE_DIR)/
	cp $(Y_ROOT)/include/yos.h $(TARGET_INCLUDE_DIR)/yos.h

stage-target-assets:
	rm -rf $(Z80_DIST_DIR)
	mkdir -p $(Z80_BIN_DIR)
	mkdir -p $(ZX_TARGET_BIN_DIR) $(ZX_TARGET_INCLUDE_DIR) $(ZX_TARGET_LIB_OUT_DIR)
	mkdir -p $(ZX_APPS_DIR) $(ZX_MDR_DIR)
	mkdir -p $(PKG_DIR)/deb $(PKG_DIR)/vsix
	cp $(Y_ROOT)/tests/microdrives/hello.mdr $(ZX_MDR_DIR)/

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
	mkdir -p $(TARGET_LIB_DIR) $(TOOLCHAIN_RUNTIME_BUILD_DIR) $(TOOLCHAIN_FIXED_BUILD_DIR)
	@for src in $$(find $(X_ROOT)/runtime -name '*.s' \
	        ! -path '*/8_8/*' \
	        ! -path '*/16_16/*' \
	        ! -path '*/24_8/*' | sort); do \
	    rel="$(TOOLCHAIN_RUNTIME_BUILD_DIR)/$$(basename "$${src%.s}").rel"; \
	    base="$$(basename "$${src%.s}")"; \
	    skip=0; \
	    case "$$base" in \
	        cabsf|cargf|cimag|conjf|creal|complex_i) \
	            skip=1 ;; \
	    esac; \
	    case "$$base" in \
	        fs*|*2fs|fs2*|fh*|*2fh|fh2*|fp*|fitosf|fstoi|ieee16_*) \
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
	@runtime_manifest="$(TOOLCHAIN_RUNTIME_BUILD_DIR)/$(RUNTIME_ARCHIVE_NAME).members"; \
	find "$(TOOLCHAIN_RUNTIME_BUILD_DIR)" -maxdepth 1 -name '*.rel' | sort > "$$runtime_manifest"; \
	test -s "$$runtime_manifest" || { echo "no runtime objects for $(RUNTIME_ARCHIVE_NAME)"; exit 1; }; \
	$(HOST_BIN_DIR)/xar --mode=gnu rcs $(TARGET_LIB_DIR)/$(RUNTIME_ARCHIVE_NAME) "@$$runtime_manifest"
	@for src in $$(find \
	        $(X_ROOT)/runtime/8_8 \
	        $(X_ROOT)/runtime/16_16 \
	        $(X_ROOT)/runtime/24_8 \
	        -name '*.s' | sort); do \
	    stem="$$(basename "$${src%.*}")"; \
	    opt="$(TOOLCHAIN_FIXED_BUILD_DIR)/$$stem.opt.s"; \
	    rel="$(TOOLCHAIN_FIXED_BUILD_DIR)/$$stem.rel"; \
	    $(HOST_BIN_DIR)/xopt -O3 "$$src" -o "$$opt"; \
	    $(HOST_BIN_DIR)/xas $(TARGET_AS8FLAGS) "$$opt" -o "$$rel"; \
	done
	@fixed_manifest="$(TOOLCHAIN_FIXED_BUILD_DIR)/$(FIXED_ARCHIVE_NAME).members"; \
	find "$(TOOLCHAIN_FIXED_BUILD_DIR)" -maxdepth 1 -name '*.rel' | sort > "$$fixed_manifest"; \
	test -s "$$fixed_manifest" || { echo "no runtime objects for $(FIXED_ARCHIVE_NAME)"; exit 1; }; \
	$(HOST_BIN_DIR)/xar --mode=gnu rcs $(TARGET_LIB_DIR)/$(FIXED_ARCHIVE_NAME) "@$$fixed_manifest"
	@for platform in $(STAGED_PLATFORMS); do \
	    sysdir="$(X_ROOT)/platforms/$$platform"; \
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
	    manifest="$$builddir/lib$$platform.members"; \
	    find "$$builddir" -maxdepth 1 -name '*.rel' | sort > "$$manifest"; \
	    test -s "$$manifest" || { echo "no platform objects for $$platform"; exit 1; }; \
	    $(HOST_BIN_DIR)/xar --mode=gnu rcs "$(TARGET_LIB_DIR)/lib$$platform.a" "@$$manifest"; \
	done
	$(MAKE) -C $(X_ROOT)/libc \
	    ROOT=$(X_ROOT) \
	    BUILD_DIR=$(BUILD_DIR) \
	    DIST_DIR=$(DIST_DIR) \
	    HOST_BIN_DIR=$(HOST_BIN_DIR) \
	    C8FLAGS="$(TARGET_C8FLAGS)" \
	    AS8FLAGS="$(TARGET_AS8FLAGS)" \
	    LIBC_PROFILE="$(LIBC_PROFILE)" \
	    LIBC_FLOAT="$(LIBC_FLOAT)" \
	    LIBC_DOUBLE="$(LIBC_DOUBLE)" \
	    LIBC_LONG="$(LIBC_LONG)" \
	    LIBC_LONGLONG="$(LIBC_LONGLONG)" \
	    LIBC_STDIO_FLOAT="$(LIBC_STDIO_FLOAT)" \
	    X_MODEL="$(X_MODEL_NAME)"
	cp $(TARGET_LIB_DIR)/crt0-$(PLATFORM).rel $(TARGET_LIB_DIR)/crt0.rel
	cp $(TARGET_LIB_DIR)/crt0-$(PLATFORM).s $(TARGET_LIB_DIR)/crt0.s
	cp $(TARGET_LIB_DIR)/linker-$(PLATFORM).lk $(TARGET_LIB_DIR)/linker.lk
	cp $(TARGET_LIB_DIR)/linker-$(PLATFORM).ld $(TARGET_LIB_DIR)/linker.ld

stage-layout-cleanup:
	rm -f $(DIST_DIR)/bin/appmake \
	      $(DIST_DIR)/bin/microdrive \
	      $(DIST_DIR)/bin/serial \
	      $(DIST_DIR)/lib/libmicrodrive.a \
	      $(DIST_DIR)/lib/libc.a \
	      $(DIST_DIR)/lib/libruntime.a \
	      $(DIST_DIR)/lib/lib$(PLATFORM).a \
	      $(DIST_DIR)/lib/crt0.rel \
	      $(DIST_DIR)/lib/crt0.s \
	      $(DIST_DIR)/lib/linker.ld \
	      $(DIST_DIR)/lib/linker.lk

stage-dist-docs:
	mkdir -p $(DIST_DIR)
	rm -rf $(DIST_DIR)/doc $(DIST_DIR)/docs $(DIST_DIR)/extensions $(DIST_DIR)/share
	mkdir -p $(DOCS_DIR) $(PKG_DIR)/deb $(PKG_DIR)/vsix
	cp $(X_ROOT)/docs/dist/README.md $(DIST_DIR)/README.md
	cp $(X_ROOT)/docs/dist/man/*.md $(DOCS_DIR)/

clean:
	@$(MAKE) -C pkg clean REPO_ROOT=$(REPO_ROOT) DIST_DIR=$(DIST_DIR) BUILD_DIR=$(BUILD_DIR)
	@$(MAKE) -C libc clean ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(HOST_BIN_DIR)
	@$(MAKE) -C src clean REPO_ROOT=$(REPO_ROOT) X_ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(HOST_BIN_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) DEFAULT_PLATFORM=$(DEFAULT_PLATFORM)
	@$(MAKE) -C lib clean REPO_ROOT=$(REPO_ROOT) X_ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
