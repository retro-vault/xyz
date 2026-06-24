# ---------------------------------------------------------------------------
# Makefile for hello — xld integration test.
#
# Two modes:
#   Outside Docker (default): runs a single container to compile .c/.s
#                             files, then runs xld on the host.
#   Inside  Docker (IN_DOCKER=1): compiles and assembles only.
#
# Usage:
#   make          — full build (compile inside Docker, link on host)
#   make clean    — remove build artifacts
# ---------------------------------------------------------------------------

_THIS_DIR  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
X_ROOT     ?= $(abspath $(_THIS_DIR)/../../..)
REPO_ROOT  ?= $(abspath $(X_ROOT)/..)
THIS_DIR   := $(realpath .)
BUILD_DIR  := $(REPO_ROOT)/build/hello
DIST_DIR   ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
BIN_DIR   := $(ZX_APPS_DIR)/hello
XLD     := $(HOST_BIN_DIR)/xld

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

ifndef IN_DOCKER
# ---------------------------------------------------------------------------
# Host entry point: Docker for SDCC compilation, then xld on host.
# ---------------------------------------------------------------------------
.PHONY: all
all:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	docker run --rm \
	    -u $(shell id -u):$(shell id -g) \
	    -v $(REPO_ROOT):$(REPO_ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 REPO_ROOT=$(REPO_ROOT) X_ROOT=$(X_ROOT)
	$(XLD) -v -m -nostdlib -e _entry \
	    -o $(BIN_DIR)/hello.xl \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/hello.rel

else
# ---------------------------------------------------------------------------
# Inside Docker: compile C and assemble S only.
# ---------------------------------------------------------------------------

CC      := sdcc
AS      := sdasz80

CFLAGS  := --std-c11 -mz80 --nostdinc
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/hello.rel

$(BUILD_DIR)/crt0.rel: crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/hello.rel: hello.c
	$(CC) -c $(CFLAGS) -o $@ $<

endif
