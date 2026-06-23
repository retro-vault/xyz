REPO_ROOT ?= $(abspath ../..)
Y_ROOT ?= $(abspath ..)
DIST_DIR ?= $(REPO_ROOT)/bin/y
BUILD_DIR ?= $(REPO_ROOT)/build
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
PUBLIC_INCLUDE_DIR ?= $(DIST_DIR)/include

SUBDIRS := serial microdrive appmake

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ \
		ROOT=$(REPO_ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
		PUBLIC_INCLUDE_DIR=$(PUBLIC_INCLUDE_DIR)

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean \
			ROOT=$(REPO_ROOT) \
			Y_ROOT=$(Y_ROOT) \
			BUILD_DIR=$(BUILD_DIR) \
			DIST_DIR=$(DIST_DIR) \
			HOST_BIN_DIR=$(HOST_BIN_DIR) \
			PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
			PUBLIC_INCLUDE_DIR=$(PUBLIC_INCLUDE_DIR); \
	done
