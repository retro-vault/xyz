REPO_ROOT ?= $(abspath ..)
Y_ROOT := $(abspath .)
X_ROOT ?= $(REPO_ROOT)/x
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/y

.PHONY: all clean

all:
	@$(MAKE) -C src \
		ROOT=$(REPO_ROOT) \
		REPO_ROOT=$(REPO_ROOT) \
		X_ROOT=$(X_ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)/yos \
		DIST_DIR=$(DIST_DIR)

clean:
	@$(MAKE) -C src clean \
		ROOT=$(REPO_ROOT) \
		REPO_ROOT=$(REPO_ROOT) \
		X_ROOT=$(X_ROOT) \
		Y_ROOT=$(Y_ROOT) \
		BUILD_DIR=$(BUILD_DIR)/yos \
		DIST_DIR=$(DIST_DIR)
