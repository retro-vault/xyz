# ---------------------------------------------------------------------------
# Makefile for hello-yos — RST10 service-query integration test.
#
# Builds a relocatable XL user app that obtains the "yos" service and prints.
# Uses the shared vanilla runtime in x/tests/hello/crt0.s.
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
Y_ROOT    ?= $(ROOT)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(realpath ../../..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR := $(REPO_ROOT)/build/hello-yos
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/hello-yos
XLD     := $(HOST_BIN_DIR)/xld
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
APPMAKE   := $(Y_HOST_BIN_DIR)/appmake
IMAGE     := $(ZX_MDR_DIR)/hello.mdr

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
	    -o $(BIN_DIR)/hello-yos.xl \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/hello-yos.rel

.PHONY: image
image: all
	@mkdir -p $(ZX_MDR_DIR)
	cp $(BIN_DIR)/hello-yos.xl $(BIN_DIR)/hello.app
	$(MDR_TOOL) create $(IMAGE) HELLO
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/hello.app
	$(MDR_TOOL) dir $(IMAGE)

else

CC      := sdcc
AS      := sdasz80

CFLAGS  := --std-c11 -mz80 --nostdinc \
           -I$(Y_ROOT)/include \
           -I$(Y_ROOT)/src \
           -I$(Y_ROOT)/src/include
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/hello-yos.rel

$(BUILD_DIR)/crt0.rel: $(X_ROOT)/tests/hello/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/hello-yos.rel: hello-yos.c
	$(CC) -c $(CFLAGS) -o $@ $<

endif
