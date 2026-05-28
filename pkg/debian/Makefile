ROOT ?= $(realpath ../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
PACKAGE_NAME ?= xtools
PACKAGE_VERSION ?= 0.1.0
PACKAGE_RELEASE ?= 1
DEB_ARCH ?= $(shell dpkg --print-architecture)
XTOOLS_INCLUDE_DIR := $(ROOT)/include
XBFD_INCLUDE_DIR := $(ROOT)/lib/xbfd/include/xbfd
XLINK_INCLUDE_DIR := $(ROOT)/src/xc/xlink/include/xlink
XCC_INCLUDE_DIR := $(ROOT)/src/xc/xcc/lib/include
XCC_RUNTIME_SRC_DIR := $(ROOT)/src/xc/xcc/lib/runtime
PKG_BUILD_ROOT := $(BUILD_DIR)/pkg/debian
PKG_STAGING_DIR := $(PKG_BUILD_ROOT)/$(PACKAGE_NAME)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(DEB_ARCH)
PKG_OUTPUT_DIR := $(DIST_DIR)/pkg/deb
PKG_OUTPUT := $(PKG_OUTPUT_DIR)/$(PACKAGE_NAME)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(DEB_ARCH).deb
CONTROL_TEMPLATE := $(ROOT)/pkg/debian/control.in
XBFD_LIB := $(BUILD_DIR)/lib/libxbfd.a
XCC_PACKAGE_BUILD_DIR := $(PKG_BUILD_ROOT)/xcc-build
XCC_PACKAGE_DIST_DIR := $(PKG_BUILD_ROOT)/xcc-dist
XCC_PACKAGE_BIN := $(XCC_PACKAGE_DIST_DIR)/bin/xcc
XCC_RUNTIME_BUILD_DIR := $(PKG_BUILD_ROOT)/xcc-runtime
XCC_RUNTIME_SOURCES := $(sort $(wildcard $(XCC_RUNTIME_SRC_DIR)/*.s))
XCC_RUNTIME_RELS := $(patsubst $(XCC_RUNTIME_SRC_DIR)/%.s,$(XCC_RUNTIME_BUILD_DIR)/%.rel,$(XCC_RUNTIME_SOURCES))
XCC_RUNTIME_ARCHIVE := $(XCC_RUNTIME_BUILD_DIR)/z80.lib
XCC_RUNTIME_COMPAT_ARCHIVE := $(XCC_RUNTIME_BUILD_DIR)/runtime.lib
HOST_TOOLS := \
	$(DIST_DIR)/bin/xlink \
	$(DIST_DIR)/bin/xdbg \
	$(DIST_DIR)/bin/xdbg-z80 \
	$(DIST_DIR)/bin/xas \
	$(DIST_DIR)/bin/xar
DEBUG_LIBS := \
	$(DIST_DIR)/lib/libxdbg.a \
	$(DIST_DIR)/lib/libxdbg_cli.a \
	$(DIST_DIR)/lib/libxdbg_dap.a \
	$(DIST_DIR)/lib/libxdbg_mi.a \
	$(DIST_DIR)/lib/libxdbgstub.a

.PHONY: all clean

all: $(PKG_OUTPUT)

$(DIST_DIR)/README.md:
	$(MAKE) -C $(ROOT) stage-dist-docs

$(DIST_DIR)/bin/xlink:
	$(MAKE) -C $(ROOT)/src/xc/xlink

$(DIST_DIR)/bin/xas:
	$(MAKE) -C $(ROOT)/src/xc/xas

$(DIST_DIR)/bin/xar:
	$(MAKE) -C $(ROOT)/src/xc/xar

$(DIST_DIR)/bin/xdbg $(DIST_DIR)/bin/xdbg-z80 $(DEBUG_LIBS):
	$(MAKE) -C $(ROOT)/src/xc/xdbg

$(XBFD_LIB):
	$(MAKE) -C $(ROOT)/lib/xbfd

$(XCC_PACKAGE_BIN):
	$(MAKE) -C $(ROOT)/src/xc/xcc clean BUILD_DIR=$(XCC_PACKAGE_BUILD_DIR) DIST_DIR=$(XCC_PACKAGE_DIST_DIR)
	$(MAKE) -C $(ROOT)/src/xc/xcc BUILD=release BUILD_DIR=$(XCC_PACKAGE_BUILD_DIR) DIST_DIR=$(XCC_PACKAGE_DIST_DIR)

$(XCC_RUNTIME_BUILD_DIR):
	mkdir -p $@

$(XCC_RUNTIME_BUILD_DIR)/%.rel: $(XCC_RUNTIME_SRC_DIR)/%.s $(DIST_DIR)/bin/xas | $(XCC_RUNTIME_BUILD_DIR)
	$(DIST_DIR)/bin/xas --mode=sdcc $< -o $@

$(XCC_RUNTIME_ARCHIVE): $(XCC_RUNTIME_RELS) $(DIST_DIR)/bin/xar | $(XCC_RUNTIME_BUILD_DIR)
	rm -f $@
	$(DIST_DIR)/bin/xar rcs $@ $(XCC_RUNTIME_RELS)

$(XCC_RUNTIME_COMPAT_ARCHIVE): $(XCC_RUNTIME_ARCHIVE)
	cp $< $@

$(PKG_OUTPUT): $(DIST_DIR)/README.md $(HOST_TOOLS) $(DEBUG_LIBS) $(XBFD_LIB) $(XCC_PACKAGE_BIN) $(XCC_RUNTIME_RELS) $(XCC_RUNTIME_ARCHIVE) $(XCC_RUNTIME_COMPAT_ARCHIVE) $(CONTROL_TEMPLATE)
	@command -v dpkg-deb >/dev/null 2>&1 || { echo "dpkg-deb not found. Please install dpkg-dev or dpkg."; exit 1; }
	rm -rf $(PKG_STAGING_DIR)
	mkdir -p $(PKG_STAGING_DIR)/DEBIAN
	mkdir -p $(PKG_STAGING_DIR)/usr/bin
	mkdir -p $(PKG_STAGING_DIR)/usr/include
	mkdir -p $(PKG_STAGING_DIR)/usr/lib
	mkdir -p $(PKG_STAGING_DIR)/usr/lib/xcc/include
	mkdir -p $(PKG_STAGING_DIR)/usr/lib/xcc/runtime
	mkdir -p $(PKG_STAGING_DIR)/usr/share/doc/$(PACKAGE_NAME)
	mkdir -p $(PKG_STAGING_DIR)/usr/share/$(PACKAGE_NAME)
	sed \
		-e 's/@PACKAGE_NAME@/$(PACKAGE_NAME)/g' \
		-e 's/@PACKAGE_VERSION@/$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)/g' \
		-e 's/@DEB_ARCH@/$(DEB_ARCH)/g' \
		$(CONTROL_TEMPLATE) > $(PKG_STAGING_DIR)/DEBIAN/control
	printf '\n' >> $(PKG_STAGING_DIR)/DEBIAN/control
	cp $(DIST_DIR)/bin/xlink $(PKG_STAGING_DIR)/usr/bin/
	cp $(DIST_DIR)/bin/xdbg $(PKG_STAGING_DIR)/usr/bin/
	cp $(DIST_DIR)/bin/xdbg-z80 $(PKG_STAGING_DIR)/usr/bin/
	cp $(DIST_DIR)/bin/xas $(PKG_STAGING_DIR)/usr/bin/
	cp $(DIST_DIR)/bin/xar $(PKG_STAGING_DIR)/usr/bin/
	cp $(XCC_PACKAGE_BIN) $(PKG_STAGING_DIR)/usr/bin/xcc
	mkdir -p $(PKG_STAGING_DIR)/usr/include/xdbg
	mkdir -p $(PKG_STAGING_DIR)/usr/include/xdbgstub
	mkdir -p $(PKG_STAGING_DIR)/usr/include/xlink
	mkdir -p $(PKG_STAGING_DIR)/usr/include/xbfd
	cp -R $(XTOOLS_INCLUDE_DIR)/xdbg/. $(PKG_STAGING_DIR)/usr/include/xdbg/
	cp -R $(XTOOLS_INCLUDE_DIR)/xdbgstub/. $(PKG_STAGING_DIR)/usr/include/xdbgstub/
	cp -R $(XLINK_INCLUDE_DIR)/. $(PKG_STAGING_DIR)/usr/include/xlink/
	cp -R $(XBFD_INCLUDE_DIR)/. $(PKG_STAGING_DIR)/usr/include/xbfd/
	cp -R $(XCC_INCLUDE_DIR)/. $(PKG_STAGING_DIR)/usr/lib/xcc/include/
	ln -s ../lib/xcc/include $(PKG_STAGING_DIR)/usr/include/xcc
	cp $(DIST_DIR)/lib/libxdbg.a $(PKG_STAGING_DIR)/usr/lib/
	cp $(DIST_DIR)/lib/libxdbg_cli.a $(PKG_STAGING_DIR)/usr/lib/
	cp $(DIST_DIR)/lib/libxdbg_dap.a $(PKG_STAGING_DIR)/usr/lib/
	cp $(DIST_DIR)/lib/libxdbg_mi.a $(PKG_STAGING_DIR)/usr/lib/
	cp $(DIST_DIR)/lib/libxdbgstub.a $(PKG_STAGING_DIR)/usr/lib/
	cp $(XBFD_LIB) $(PKG_STAGING_DIR)/usr/lib/
	cp $(XCC_RUNTIME_RELS) $(PKG_STAGING_DIR)/usr/lib/xcc/runtime/
	cp $(XCC_RUNTIME_ARCHIVE) $(PKG_STAGING_DIR)/usr/lib/xcc/runtime/
	cp $(XCC_RUNTIME_COMPAT_ARCHIVE) $(PKG_STAGING_DIR)/usr/lib/xcc/runtime/
	cp $(DIST_DIR)/README.md $(PKG_STAGING_DIR)/usr/share/doc/$(PACKAGE_NAME)/README.md
	cp $(ROOT)/src/xc/xcc/README.md $(PKG_STAGING_DIR)/usr/share/doc/$(PACKAGE_NAME)/XCC.md
	if [ -d "$(DIST_DIR)/docs" ]; then cp -R $(DIST_DIR)/docs/. $(PKG_STAGING_DIR)/usr/share/doc/$(PACKAGE_NAME)/; fi
	if [ -d "$(DIST_DIR)/extensions" ]; then cp -R $(DIST_DIR)/extensions $(PKG_STAGING_DIR)/usr/share/$(PACKAGE_NAME)/; fi
	mkdir -p $(PKG_OUTPUT_DIR)
	dpkg-deb --build $(PKG_STAGING_DIR) $(PKG_OUTPUT)

clean:
	rm -rf $(PKG_BUILD_ROOT) $(PKG_OUTPUT_DIR)
