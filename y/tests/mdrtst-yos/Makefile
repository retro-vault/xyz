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
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/mdrtst-yos
XLD     := $(HOST_BIN_DIR)/xld
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
IMAGE     := $(ZX_MDR_DIR)/mdrtst.mdr

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(IMAGE)

ifndef IN_DOCKER
.PHONY: all
all:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	docker run --rm \
	    -u $(shell id -u):$(shell id -g) \
	    -v $(REPO_ROOT):$(REPO_ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 ROOT=$(ROOT) REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) X_ROOT=$(X_ROOT)
	$(XLD) -v -m -nostdlib -e _entry \
	    -o $(BIN_DIR)/mdrtst.app \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/mdrtst.rel

.PHONY: image
image: all
	@mkdir -p $(ZX_MDR_DIR)
	$(MDR_TOOL) create $(IMAGE) MDRTST
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/mdrtst.app
	$(MDR_TOOL) dir $(IMAGE)

else

AS      := sdasz80
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/mdrtst.rel

$(BUILD_DIR)/crt0.rel: $(X_ROOT)/tests/hello/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/mdrtst.rel: mdrtst.s
	$(AS) $(ASFLAGS) -o $@ $<

endif
