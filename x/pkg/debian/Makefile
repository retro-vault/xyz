THIS_DIR := $(realpath .)
ROOT ?= $(realpath ../../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
X_ROOT ?= $(ROOT)/x
Y_ROOT ?= $(ROOT)/y
PACKAGE_NAME ?= x
include $(X_ROOT)/mk/model.mk
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
PACKAGE_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
PACKAGE_RELEASE ?= 1
DEB_ARCH ?= $(shell dpkg --print-architecture)
INSTALL_PREFIX ?= /opt/$(PACKAGE_NAME)
PROFILE_D_DIR ?= /etc/profile.d
PROFILE_SCRIPT_NAME ?= $(PACKAGE_NAME).sh
DEFAULT_PLATFORM ?= none
LIBC_PROFILE ?= full
LIBC_FLOAT ?= $(X_MODEL_LIBC_FLOAT_DEFAULT)
LIBC_DOUBLE ?= $(X_MODEL_LIBC_DOUBLE_DEFAULT)
LIBC_LONG ?= $(X_MODEL_LIBC_LONG_DEFAULT)
LIBC_LONGLONG ?= $(X_MODEL_LIBC_LONGLONG_DEFAULT)
LIBC_STDIO_FLOAT ?= $(X_MODEL_LIBC_STDIO_FLOAT_DEFAULT)
ifeq ($(LIBC_FLOAT),0)
override LIBC_STDIO_FLOAT := 0
endif
XGDB_INCLUDE_DIR := $(X_ROOT)/lib/xgdb/include/xgdb
XOPT_INCLUDE_DIR := $(X_ROOT)/lib/xopt/include/xopt
RSP_INCLUDE_DIR := $(X_ROOT)/lib/rsp/include/rsp
XBFD_INCLUDE_DIR := $(X_ROOT)/lib/xbfd/include/xbfd
XLINK_INCLUDE_DIR := $(X_ROOT)/src/xld/include/xld
XCC_INCLUDE_DIR := $(X_ROOT)/libc/include
XCC_RUNTIME_SRC_DIR := $(X_ROOT)/runtime
PKG_BUILD_ROOT := $(BUILD_DIR)/x/pkg/debian
PKG_STAGING_DIR := $(PKG_BUILD_ROOT)/$(PACKAGE_NAME)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(DEB_ARCH)
PKG_OUTPUT_DIR := $(DIST_DIR)/pkg/deb
PKG_OUTPUT := $(PKG_OUTPUT_DIR)/$(PACKAGE_NAME)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(DEB_ARCH).deb
CONTROL_TEMPLATE := $(THIS_DIR)/control.in
DIST_DOC_SOURCES := $(X_ROOT)/docs/dist/README.md $(wildcard $(X_ROOT)/docs/dist/man/*.md)
DIST_DOC_STAMP := $(DIST_DIR)/.stage-dist-docs.stamp
XBFD_LIB := $(BUILD_DIR)/lib/libxbfd.a
XOPT_LIB := $(DIST_DIR)/lib/libxopt.a
XCC_PACKAGE_BUILD_DIR := $(PKG_BUILD_ROOT)/xcc-build
XCC_PACKAGE_DIST_DIR := $(PKG_BUILD_ROOT)/xcc-dist
XCC_PACKAGE_BIN := $(XCC_PACKAGE_DIST_DIR)/bin/xcc
XCC_RUNTIME_BUILD_DIR := $(PKG_BUILD_ROOT)/xcc-runtime
XCC_RUNTIME_SOURCES := $(shell find $(XCC_RUNTIME_SRC_DIR) -type f -name '*.s' | sort)
XCC_RUNTIME_RELS := $(patsubst $(XCC_RUNTIME_SRC_DIR)/%.s,$(XCC_RUNTIME_BUILD_DIR)/%.rel,$(XCC_RUNTIME_SOURCES))
XCC_RUNTIME_ARCHIVE := $(XCC_RUNTIME_BUILD_DIR)/z80.lib
XCC_RUNTIME_COMPAT_ARCHIVE := $(XCC_RUNTIME_BUILD_DIR)/runtime.lib
XCC_RUNTIME_MANIFEST := $(XCC_RUNTIME_BUILD_DIR)/z80-lib.members
DIST_PREFIX_FILES := \
	$(DIST_DIR)/bin/xcc \
	$(DIST_DIR)/z80/include/stdio.h \
	$(DIST_DIR)/z80/lib/crt0-none.rel \
	$(DIST_DIR)/z80/lib/crt0-cpm3.rel \
	$(DIST_DIR)/z80/lib/libc.a \
	$(DIST_DIR)/z80/lib/libfixed.a \
	$(DIST_DIR)/z80/lib/libnone.a \
	$(DIST_DIR)/z80/lib/libcpm3.a \
	$(DIST_DIR)/z80/lib/libruntime.a
HOST_TOOLS := \
	$(DIST_DIR)/bin/xld \
	$(DIST_DIR)/bin/xgdb \
	$(DIST_DIR)/bin/xemu \
	$(DIST_DIR)/bin/xas \
	$(DIST_DIR)/bin/xar \
	$(DIST_DIR)/bin/xobjcopy \
	$(DIST_DIR)/bin/xprog \
	$(DIST_DIR)/bin/xopt
DEBUG_LIBS := \
	$(DIST_DIR)/lib/libxgdb.a \
	$(DIST_DIR)/lib/libxgdb_cli.a \
	$(DIST_DIR)/lib/libxgdb_mi.a \
	$(DIST_DIR)/lib/librsp.a
EMU_LIBS := \
	$(DIST_DIR)/lib/libxemu.a \
	$(DIST_DIR)/lib/libxz80.a

.PHONY: all clean

all: $(PKG_OUTPUT)

$(DIST_DOC_STAMP): $(DIST_DOC_SOURCES)
	$(MAKE) -C $(X_ROOT) stage-dist-docs REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) OUT_DIR=$(BUILD_DIR)/out Z_DIST_DIR=$(BUILD_DIR)/z
	@touch $@

$(DIST_DIR)/bin/xld:
	$(MAKE) -C $(X_ROOT)/src/xld ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin DEFAULT_PLATFORM=$(DEFAULT_PLATFORM)

$(DIST_DIR)/bin/xas:
	$(MAKE) -C $(X_ROOT)/src/xas ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin

$(DIST_DIR)/bin/xar:
	$(MAKE) -C $(X_ROOT)/src/xar ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin

$(DIST_DIR)/bin/xobjcopy:
	$(MAKE) -C $(X_ROOT)/src/xobjcopy ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin

$(DIST_DIR)/bin/xprog:
	$(MAKE) -C $(X_ROOT)/src/xprog ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin

$(DIST_DIR)/bin/xopt $(XOPT_LIB):
	$(MAKE) -C $(X_ROOT)/src/xopt ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin PUBLIC_LIB_DIR=$(DIST_DIR)/lib

$(DIST_DIR)/bin/xgdb $(DEBUG_LIBS):
	$(MAKE) -C $(X_ROOT)/src/xgdb ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin PUBLIC_LIB_DIR=$(DIST_DIR)/lib

$(DIST_DIR)/bin/xemu $(EMU_LIBS):
	$(MAKE) -C $(X_ROOT)/src/xemu ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(DIST_DIR)/bin PUBLIC_LIB_DIR=$(DIST_DIR)/lib

$(XBFD_LIB):
	$(MAKE) -C $(X_ROOT)/lib/xbfd ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR)

$(XCC_PACKAGE_BIN):
	$(MAKE) -C $(X_ROOT)/src/xcc clean ROOT=$(X_ROOT) BUILD_DIR=$(XCC_PACKAGE_BUILD_DIR) DIST_DIR=$(XCC_PACKAGE_DIST_DIR) HOST_BIN_DIR=$(XCC_PACKAGE_DIST_DIR)/bin XBFD_LIB=$(XBFD_LIB) XOPT_LIB=$(BUILD_DIR)/lib/libxopt.a
	$(MAKE) -C $(X_ROOT)/src/xcc BUILD=release ROOT=$(X_ROOT) BUILD_DIR=$(XCC_PACKAGE_BUILD_DIR) DIST_DIR=$(XCC_PACKAGE_DIST_DIR) HOST_BIN_DIR=$(XCC_PACKAGE_DIST_DIR)/bin XBFD_LIB=$(XBFD_LIB) XOPT_LIB=$(BUILD_DIR)/lib/libxopt.a

$(XCC_RUNTIME_BUILD_DIR):
	mkdir -p $@

$(XCC_RUNTIME_BUILD_DIR)/%.rel: $(XCC_RUNTIME_SRC_DIR)/%.s $(DIST_DIR)/bin/xas | $(XCC_RUNTIME_BUILD_DIR)
	mkdir -p $(dir $@)
	$(DIST_DIR)/bin/xas --mode=sdcc $< -o $@

$(XCC_RUNTIME_ARCHIVE): $(XCC_RUNTIME_RELS) $(XCC_RUNTIME_MANIFEST) $(DIST_DIR)/bin/xar | $(XCC_RUNTIME_BUILD_DIR)
	rm -f $@
	$(DIST_DIR)/bin/xar rcs $@ @$(XCC_RUNTIME_MANIFEST)

$(XCC_RUNTIME_MANIFEST): $(XCC_RUNTIME_RELS) | $(XCC_RUNTIME_BUILD_DIR)
	find "$(XCC_RUNTIME_BUILD_DIR)" -type f -name '*.rel' | LC_ALL=C sort > $@
	test -s $@

$(XCC_RUNTIME_COMPAT_ARCHIVE): $(XCC_RUNTIME_ARCHIVE)
	cp $< $@

$(DIST_PREFIX_FILES):
	$(MAKE) -C $(X_ROOT) x REPO_ROOT=$(ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) OUT_DIR=$(BUILD_DIR)/out Z_DIST_DIR=$(BUILD_DIR)/z PLATFORM=$(DEFAULT_PLATFORM) DEFAULT_PLATFORM=$(DEFAULT_PLATFORM) LIBC_PROFILE=$(LIBC_PROFILE) LIBC_FLOAT=$(LIBC_FLOAT) LIBC_DOUBLE=$(LIBC_DOUBLE) LIBC_LONG=$(LIBC_LONG) LIBC_LONGLONG=$(LIBC_LONGLONG) LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT) X_MODEL=$(X_MODEL_NAME)

$(PKG_OUTPUT): $(DIST_DOC_STAMP) $(HOST_TOOLS) $(DEBUG_LIBS) $(EMU_LIBS) $(XBFD_LIB) $(XOPT_LIB) $(XCC_PACKAGE_BIN) $(DIST_PREFIX_FILES) $(CONTROL_TEMPLATE)
	@command -v dpkg-deb >/dev/null 2>&1 || { echo "dpkg-deb not found. Please install dpkg-dev or dpkg."; exit 1; }
	rm -rf $(PKG_STAGING_DIR)
	mkdir -p $(PKG_STAGING_DIR)/DEBIAN
	mkdir -p $(PKG_STAGING_DIR)$(INSTALL_PREFIX)
	mkdir -p $(PKG_STAGING_DIR)$(PROFILE_D_DIR)
	sed \
		-e 's/@PACKAGE_NAME@/$(PACKAGE_NAME)/g' \
		-e 's/@PACKAGE_VERSION@/$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)/g' \
		-e 's/@DEB_ARCH@/$(DEB_ARCH)/g' \
		-e 's#@INSTALL_PREFIX@#$(INSTALL_PREFIX)#g' \
		-e 's#@PROFILE_SCRIPT@#$(PROFILE_D_DIR)/$(PROFILE_SCRIPT_NAME)#g' \
		$(CONTROL_TEMPLATE) > $(PKG_STAGING_DIR)/DEBIAN/control
	printf '\n' >> $(PKG_STAGING_DIR)/DEBIAN/control
	cp -a $(DIST_DIR)/. $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/
	rm -rf $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/pkg
	rm -f $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/z80/lib/crt0-emu.rel \
	      $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/z80/lib/crt0-emu.s \
	      $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/z80/lib/libemu.a \
	      $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/z80/lib/libemu.lib \
	      $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/z80/lib/linker-emu.ld \
	      $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/z80/lib/linker-emu.lk
	cp $(XCC_PACKAGE_BIN) $(PKG_STAGING_DIR)$(INSTALL_PREFIX)/bin/xcc
	printf '%s\n' \
		'# $(PACKAGE_NAME) compiler suite for z80' \
		'case ":$${PATH}:" in' \
		'  *:$(INSTALL_PREFIX)/bin:*) ;;' \
		'  *) export PATH="$(INSTALL_PREFIX)/bin:$${PATH}" ;;' \
		'esac' \
		> $(PKG_STAGING_DIR)$(PROFILE_D_DIR)/$(PROFILE_SCRIPT_NAME)
	chmod 0644 $(PKG_STAGING_DIR)$(PROFILE_D_DIR)/$(PROFILE_SCRIPT_NAME)
	mkdir -p $(PKG_OUTPUT_DIR)
	dpkg-deb --build $(PKG_STAGING_DIR) $(PKG_OUTPUT)

clean:
	rm -rf $(PKG_BUILD_ROOT) $(PKG_OUTPUT_DIR)
