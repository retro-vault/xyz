ROOT      ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
BUILD_DIR := $(ROOT)/build/debug
DIST_DIR  ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
Z80_APPS_DIR ?= $(DIST_DIR)/targets/z80/apps
BIN_DIR   := $(Z80_APPS_DIR)/debug
XLINK     := $(HOST_BIN_DIR)/xlink

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

ifndef IN_DOCKER

.PHONY: all
all:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(MAKE) -C $(ROOT)/src/xc/xlink all
	docker run --rm \
	    -u $(shell id -u):$(shell id -g) \
	    -v $(ROOT):$(ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 ROOT=$(ROOT)
	$(XLINK) -f bin -e _entry -b _CODE=0100 -x 0100-02FF \
	    -g $(BIN_DIR)/sieve.xdbg \
	    -o $(BIN_DIR)/sieve.bin \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/sieve.rel

else

CC      := sdcc
AS      := sdasz80
CFLAGS  := --std-c11 -mz80 --debug --nostdinc
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/sieve.rel

$(BUILD_DIR)/crt0.rel: crt0.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/sieve.rel: sieve.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

endif
