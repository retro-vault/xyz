# ---------------------------------------------------------------------------
# Makefile for mdrtst-yos — standalone microdrive save/load self-test app.
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
Y_ROOT    ?= $(ROOT)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(realpath ../../..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR := $(REPO_ROOT)/build/mdrtst-yos
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
X_DIST_DIR ?= $(REPO_ROOT)/bin/x
X_HOST_BIN_DIR ?= $(X_DIST_DIR)/bin
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/mdrtst-yos
XAS     := $(X_HOST_BIN_DIR)/xas
XLD     := $(X_HOST_BIN_DIR)/xld
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
IMAGE     := $(ZX_MDR_DIR)/mdrtst.mdr

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(IMAGE)

.PHONY: all
all: $(BIN_DIR)/mdrtst.app

XASFLAGS := --mode=sdcc -g
XLDFLAGS := --mode=sdcc -v -m -nostdlib --no-default-runtime -e _entry

$(XAS) $(XLD):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/mdrtst.app: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/mdrtst.rel $(XLD) | $(BIN_DIR)
	$(XLD) $(XLDFLAGS) \
	    -o $(BIN_DIR)/mdrtst.app \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/mdrtst.rel

.PHONY: image
image: all $(MDR_TOOL)
	@mkdir -p $(ZX_MDR_DIR)
	$(MDR_TOOL) create $(IMAGE) MDRTST
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/mdrtst.app
	$(MDR_TOOL) dir $(IMAGE)

$(MDR_TOOL):
	$(MAKE) -C $(Y_ROOT)/pkg/microdrive all \
		ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(REPO_ROOT)/build \
		DIST_DIR=$(Y_DIST_DIR) HOST_BIN_DIR=$(Y_HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include

$(BUILD_DIR)/crt0.rel: $(X_ROOT)/tests/tests/hello/crt0.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/mdrtst.rel: mdrtst.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@
