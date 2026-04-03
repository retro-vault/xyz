# ---------------------------------------------------------------------------
# Makefile for hello-yos — RST10 service-query integration test.
#
# Builds a relocatable XL user app that obtains the "yos" service and prints.
# Uses the shared vanilla runtime in tests/hello/crt0.s.
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
BUILD_DIR := $(ROOT)/build/hello-yos
BIN_DIR   := $(ROOT)/bin/hello-yos
XLINK     := $(ROOT)/bin/xc/xlink/xlink
MDR_TOOL  := $(ROOT)/bin/tools/microdrive/mdr
IMAGE     := $(ROOT)/tests/microdrives/hello.mdr

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

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
	    -o $(BIN_DIR)/hello-yos.xl \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/hello-yos.rel

.PHONY: image
image: all
	cp $(BIN_DIR)/hello-yos.xl $(BIN_DIR)/hello.app
	$(MDR_TOOL) create $(IMAGE) HELLO
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/hello.app
	$(MDR_TOOL) dir $(IMAGE)

else

CC      := sdcc
AS      := sdasz80

CFLAGS  := --std-c11 -mz80 --nostdinc \
           -I$(ROOT)/include \
           -I$(ROOT)/src/yos \
           -I$(ROOT)/src/yos/include
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/hello-yos.rel

$(BUILD_DIR)/crt0.rel: ../hello/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/hello-yos.rel: hello-yos.c
	$(CC) -c $(CFLAGS) -o $@ $<

endif
