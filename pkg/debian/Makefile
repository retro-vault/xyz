ROOT ?= $(realpath ../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
PACKAGE_NAME ?= xtools
PACKAGE_VERSION ?= 0.1.0
PACKAGE_RELEASE ?= 1
DEB_ARCH ?= $(shell dpkg --print-architecture)
PKG_BUILD_ROOT := $(BUILD_DIR)/pkg/debian
PKG_STAGING_DIR := $(PKG_BUILD_ROOT)/$(PACKAGE_NAME)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(DEB_ARCH)
PKG_OUTPUT_DIR := $(DIST_DIR)/pkg/deb
PKG_OUTPUT := $(PKG_OUTPUT_DIR)/$(PACKAGE_NAME)_$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)_$(DEB_ARCH).deb
CONTROL_TEMPLATE := $(ROOT)/pkg/debian/control.in

.PHONY: all clean

all: $(PKG_OUTPUT)

$(PKG_OUTPUT): $(DIST_DIR)/README.md $(DIST_DIR)/bin/xlink $(DIST_DIR)/bin/xdbg $(DIST_DIR)/bin/xdbg-z80 $(CONTROL_TEMPLATE)
	@command -v dpkg-deb >/dev/null 2>&1 || { echo "dpkg-deb not found. Please install dpkg-dev or dpkg."; exit 1; }
	rm -rf $(PKG_STAGING_DIR)
	mkdir -p $(PKG_STAGING_DIR)/DEBIAN
	mkdir -p $(PKG_STAGING_DIR)/usr/bin
	mkdir -p $(PKG_STAGING_DIR)/usr/include
	mkdir -p $(PKG_STAGING_DIR)/usr/lib
	mkdir -p $(PKG_STAGING_DIR)/usr/share/doc/$(PACKAGE_NAME)
	mkdir -p $(PKG_STAGING_DIR)/usr/share/$(PACKAGE_NAME)
	sed \
		-e 's/@PACKAGE_NAME@/$(PACKAGE_NAME)/g' \
		-e 's/@PACKAGE_VERSION@/$(PACKAGE_VERSION)-$(PACKAGE_RELEASE)/g' \
		-e 's/@DEB_ARCH@/$(DEB_ARCH)/g' \
		$(CONTROL_TEMPLATE) > $(PKG_STAGING_DIR)/DEBIAN/control
	printf '\n' >> $(PKG_STAGING_DIR)/DEBIAN/control
	cp -R $(DIST_DIR)/bin/. $(PKG_STAGING_DIR)/usr/bin/
	cp -R $(DIST_DIR)/include/. $(PKG_STAGING_DIR)/usr/include/
	cp -R $(DIST_DIR)/lib/. $(PKG_STAGING_DIR)/usr/lib/
	cp $(DIST_DIR)/README.md $(PKG_STAGING_DIR)/usr/share/doc/$(PACKAGE_NAME)/README.md
	if [ -d "$(DIST_DIR)/extensions" ]; then cp -R $(DIST_DIR)/extensions $(PKG_STAGING_DIR)/usr/share/$(PACKAGE_NAME)/; fi
	if [ -d "$(DIST_DIR)/targets" ]; then cp -R $(DIST_DIR)/targets $(PKG_STAGING_DIR)/usr/share/$(PACKAGE_NAME)/; fi
	mkdir -p $(PKG_OUTPUT_DIR)
	dpkg-deb --build $(PKG_STAGING_DIR) $(PKG_OUTPUT)

clean:
	rm -rf $(PKG_BUILD_ROOT) $(PKG_OUTPUT_DIR)
