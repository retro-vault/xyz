REPO_ROOT ?= $(abspath ../..)
X_ROOT ?= $(abspath ..)
Y_ROOT ?= $(REPO_ROOT)/y
DIST_DIR ?= $(REPO_ROOT)/bin/x
BUILD_DIR ?= $(REPO_ROOT)/build
VSIX_STAGE_DIR ?= $(DIST_DIR)/pkg/vsix

SUBDIRS := xgdb-vsix debian

.PHONY: all clean $(SUBDIRS)

all: debian

debian: xgdb-vsix

$(SUBDIRS):
	$(MAKE) -C $@ \
		ROOT=$(REPO_ROOT) \
		X_ROOT=$(X_ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR) \
		DIST_DIR=$(DIST_DIR) \
		VSIX_STAGE_DIR=$(VSIX_STAGE_DIR)

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean \
			ROOT=$(REPO_ROOT) \
			X_ROOT=$(X_ROOT) \
			Y_ROOT=$(Y_ROOT) \
			BUILD_DIR=$(BUILD_DIR) \
			DIST_DIR=$(DIST_DIR) \
			VSIX_STAGE_DIR=$(VSIX_STAGE_DIR); \
	done
