# ---------------------------------------------------------------------------
# Makefile for mdrtst-yos — standalone microdrive save/load self-test app.
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
BUILD_DIR := $(ROOT)/build/mdrtst-yos
BIN_DIR   := $(ROOT)/bin/mdrtst-yos
XLINK     := $(ROOT)/bin/xc/xlink/xlink
MDR_TOOL  := $(ROOT)/bin/tools/microdrive/mdr
IMAGE     := $(ROOT)/tests/microdrives/mdrtst.mdr

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
	    -v $(ROOT):$(ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 ROOT=$(ROOT)
	$(XLINK) -v -m -e _entry \
	    -o $(BIN_DIR)/mdrtst.app \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/mdrtst.rel

.PHONY: image
image: all
	$(MDR_TOOL) create $(IMAGE) MDRTST
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/mdrtst.app
	$(MDR_TOOL) dir $(IMAGE)

else

AS      := sdasz80
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/mdrtst.rel

$(BUILD_DIR)/crt0.rel: ../hello/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/mdrtst.rel: mdrtst.s
	$(AS) $(ASFLAGS) -o $@ $<

endif
