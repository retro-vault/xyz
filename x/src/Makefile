REPO_ROOT ?= $(abspath ../..)
X_ROOT ?= $(abspath ..)
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
DEFAULT_PLATFORM ?= none

SUBDIRS := xld xgdb xemu xcc xopt xas xar xobjcopy

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ \
		ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
		DEFAULT_PLATFORM=$(DEFAULT_PLATFORM)

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
