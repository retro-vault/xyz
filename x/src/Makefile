REPO_ROOT ?= $(abspath ../..)
X_ROOT ?= $(abspath ..)
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
DEFAULT_PLATFORM ?= none

SUBDIRS := xld xgdb xemu xopt xcc xas xar xobjcopy

.PHONY: all clean host-libs $(SUBDIRS)

all: $(SUBDIRS)

# Build the shared host libraries once before parallel tool builds.  Several
# tools also retain their standalone recursive prerequisites, but funnelling
# the initial build through this target prevents concurrent `ar` writers when
# x/src is invoked directly with -j.
host-libs:
	$(MAKE) -C $(X_ROOT)/lib \
		ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(SUBDIRS): host-libs
	$(MAKE) -C $@ \
		ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM)

# xcc links libxopt, so even parallel product builds must finish xopt first.
xcc: xopt

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean \
			ROOT=$(X_ROOT) \
			BUILD_DIR=$(BUILD_DIR) \
			DIST_DIR=$(DIST_DIR) \
			HOST_BIN_DIR=$(HOST_BIN_DIR) \
			PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
			DEFAULT_PLATFORM=$(DEFAULT_PLATFORM); \
	done
