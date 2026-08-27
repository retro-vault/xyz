REPO_ROOT ?= $(abspath ../..)
X_ROOT ?= $(abspath ..)
Y_ROOT ?= $(REPO_ROOT)/y
DIST_DIR ?= $(REPO_ROOT)/bin/x
BUILD_DIR ?= $(REPO_ROOT)/build
VSIX_STAGE_DIR ?= $(DIST_DIR)/pkg/vsix

SUBDIRS := xgdb-vsix debian
SUBMAKE_ARGS := \
	ROOT=$(REPO_ROOT) \
	X_ROOT=$(X_ROOT) \
	Y_ROOT=$(Y_ROOT) \
	BUILD_DIR=$(BUILD_DIR) \
	DIST_DIR=$(DIST_DIR) \
	VSIX_STAGE_DIR=$(VSIX_STAGE_DIR) \
	$(if $(strip $(DEFAULT_PLATFORM)),DEFAULT_PLATFORM=$(DEFAULT_PLATFORM)) \
	$(if $(strip $(PACKAGE_NAME)),PACKAGE_NAME=$(PACKAGE_NAME)) \
	$(if $(strip $(PACKAGE_VERSION)),PACKAGE_VERSION=$(PACKAGE_VERSION)) \
	$(if $(strip $(PACKAGE_RELEASE)),PACKAGE_RELEASE=$(PACKAGE_RELEASE)) \
	$(if $(strip $(LIBC_PROFILE)),LIBC_PROFILE=$(LIBC_PROFILE)) \
	$(if $(strip $(LIBC_FLOAT)),LIBC_FLOAT=$(LIBC_FLOAT)) \
	$(if $(strip $(LIBC_DOUBLE)),LIBC_DOUBLE=$(LIBC_DOUBLE)) \
	$(if $(strip $(LIBC_LONG)),LIBC_LONG=$(LIBC_LONG)) \
	$(if $(strip $(LIBC_LONGLONG)),LIBC_LONGLONG=$(LIBC_LONGLONG)) \
	$(if $(strip $(LIBC_STDIO_FLOAT)),LIBC_STDIO_FLOAT=$(LIBC_STDIO_FLOAT)) \
	$(if $(strip $(X_MODEL)),X_MODEL=$(X_MODEL))

.PHONY: all clean $(SUBDIRS)

all: debian

debian: xgdb-vsix

$(SUBDIRS):
	$(MAKE) -C $@ $(SUBMAKE_ARGS)

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean $(SUBMAKE_ARGS); \
	done
