# ---------------------------------------------------------------------------
# Makefile for hello-yos — RST10 service-query integration test.
#
# Builds a relocatable XL user app that obtains the "yos" service and prints.
# Uses the shared vanilla runtime in x/tests/tests/hello/crt0.s.
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
Y_ROOT    ?= $(ROOT)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(realpath ../../..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR := $(REPO_ROOT)/build/hello-yos
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
X_DIST_DIR ?= $(REPO_ROOT)/bin/x
X_HOST_BIN_DIR ?= $(X_DIST_DIR)/bin
X_TARGET_LIB_DIR ?= $(X_DIST_DIR)/z80/lib
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/hello-yos
XCC     := $(X_HOST_BIN_DIR)/xcc
XAS     := $(X_HOST_BIN_DIR)/xas
XLD     := $(X_HOST_BIN_DIR)/xld
XRUNTIME := $(X_TARGET_LIB_DIR)/libruntime.a
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
APPMAKE   := $(Y_HOST_BIN_DIR)/appmake
IMAGE     := $(ZX_MDR_DIR)/hello.mdr

.DEFAULT_GOAL := all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(IMAGE)

.PHONY: all
all: $(BIN_DIR)/hello-yos.xl

XCCFLAGS := --mode=sdcc -S -Os -g -std=c11 \
           -I$(Y_ROOT)/include \
           -I$(Y_ROOT)/src \
           -I$(Y_ROOT)/src/include
XASFLAGS := --mode=sdcc -g
XLDFLAGS := --mode=sdcc -v -m -nostdlib --no-default-runtime -e _entry

$(XCC) $(XAS) $(XLD) $(XRUNTIME):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/hello-yos.xl: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/hello-yos.rel $(XLD) $(XRUNTIME) | $(BIN_DIR)
	$(XLD) $(XLDFLAGS) \
	    -o $(BIN_DIR)/hello-yos.xl \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/hello-yos.rel \
	    $(XRUNTIME)

.PHONY: image
image: all $(MDR_TOOL)
	@mkdir -p $(ZX_MDR_DIR)
	cp $(BIN_DIR)/hello-yos.xl $(BIN_DIR)/hello.app
	$(MDR_TOOL) create $(IMAGE) HELLO
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/hello.app
	$(MDR_TOOL) dir $(IMAGE)

$(MDR_TOOL):
	$(MAKE) -C $(Y_ROOT)/pkg/microdrive all \
		ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(REPO_ROOT)/build \
		DIST_DIR=$(Y_DIST_DIR) HOST_BIN_DIR=$(Y_HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include

$(BUILD_DIR)/crt0.rel: $(X_ROOT)/tests/tests/hello/crt0.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/hello-yos.s: hello-yos.c $(XCC) | $(BUILD_DIR)
	$(XCC) $(XCCFLAGS) -o $@ $<

$(BUILD_DIR)/hello-yos.rel: $(BUILD_DIR)/hello-yos.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@
