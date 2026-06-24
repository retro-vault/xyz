REPO_ROOT ?= $(abspath ../..)
X_ROOT ?= $(abspath ..)
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/x
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib

SUBDIRS := xbfd xopt xz80 rsp xgdb xemu

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ \
		ROOT=$(X_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean \
			ROOT=$(X_ROOT) \
			BUILD_DIR=$(BUILD_DIR) \
			DIST_DIR=$(DIST_DIR) \
			PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR); \
	done
