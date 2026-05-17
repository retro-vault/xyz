ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
VSIX_STAGE_DIR ?= $(DIST_DIR)/pkg/vsix
VSIX_VERSION ?= 0.1.0

VSIX_NAME := xdbg-vsix-$(VSIX_VERSION).vsix
VSIX_BUILD_DIR := $(BUILD_DIR)/tools/xdbg-vsix
VSIX_SRC_DIR := $(VSIX_BUILD_DIR)/src
VSIX_BUILD_PATH := $(VSIX_BUILD_DIR)/$(VSIX_NAME)
VSIX_STAGE_PATH := $(VSIX_STAGE_DIR)/$(VSIX_NAME)

.PHONY: all check clean

all: $(VSIX_STAGE_PATH)

check:
	node --check extension.js

$(VSIX_BUILD_DIR):
	mkdir -p $@

$(VSIX_STAGE_DIR):
	mkdir -p $@

$(VSIX_SRC_DIR): package.json extension.js README.md LICENSE.txt .vscodeignore | $(VSIX_BUILD_DIR)
	mkdir -p $@
	sed 's/"version": "[^"]*"/"version": "$(VSIX_VERSION)"/' package.json > $@/package.json
	cp extension.js README.md LICENSE.txt .vscodeignore $@/

$(VSIX_BUILD_PATH): $(VSIX_SRC_DIR)
	@test -x ./node_modules/.bin/vsce || { echo "vsce not installed. Run 'npm ci' in tools/xdbg-vsix."; exit 1; }
	cd $(VSIX_SRC_DIR) && ../../../../tools/xdbg-vsix/node_modules/.bin/vsce package --no-dependencies -o ../$(VSIX_NAME)

$(VSIX_STAGE_PATH): $(VSIX_BUILD_PATH) | $(VSIX_STAGE_DIR)
	cp $< $@

clean:
	rm -rf $(VSIX_BUILD_DIR)
	rm -f $(VSIX_STAGE_PATH)
