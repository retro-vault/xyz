ROOT      ?= $(realpath ../..)
Y_ROOT    ?= $(ROOT)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(realpath ../../..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR := $(REPO_ROOT)/build/mdrsave-yos
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
X_DIST_DIR ?= $(REPO_ROOT)/bin/x
X_HOST_BIN_DIR ?= $(X_DIST_DIR)/bin
X_TARGET_LIB_DIR ?= $(X_DIST_DIR)/z80/lib
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/mdrsave-yos
XCC     := $(X_HOST_BIN_DIR)/xcc
XAS     := $(X_HOST_BIN_DIR)/xas
XLD     := $(X_HOST_BIN_DIR)/xld
XRUNTIME := $(X_TARGET_LIB_DIR)/libruntime.a
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
IMAGE     := $(ZX_MDR_DIR)/mdrsave.mdr

.DEFAULT_GOAL := all

APPS := s123 s512 s777

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(IMAGE)

XCCFLAGS := --mode=sdcc -S -Os -g -std=c11 \
           -I$(Y_ROOT)/include \
           -I$(Y_ROOT)/src \
           -I$(Y_ROOT)/src/include
XASFLAGS := --mode=sdcc -g
XLDFLAGS := --mode=sdcc -v -m -nostdlib --no-default-runtime -e _entry

.PHONY: all
all: $(addprefix $(BIN_DIR)/,$(addsuffix .app,$(APPS)))

$(XCC) $(XAS) $(XLD) $(XRUNTIME):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/%.app: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/%.rel $(XLD) $(XRUNTIME) | $(BIN_DIR)
	$(XLD) $(XLDFLAGS) \
	    -o $@ \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/$*.rel \
	    $(XRUNTIME)

.PHONY: image
image: all $(MDR_TOOL)
	@mkdir -p $(ZX_MDR_DIR)
	$(MDR_TOOL) create $(IMAGE) MDRSAVE
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s123.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s512.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s777.app
	$(MDR_TOOL) dir $(IMAGE)

$(MDR_TOOL):
	$(MAKE) -C $(Y_ROOT)/pkg/microdrive all \
		ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(REPO_ROOT)/build \
		DIST_DIR=$(Y_DIST_DIR) HOST_BIN_DIR=$(Y_HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include

$(BUILD_DIR)/crt0.rel: $(X_ROOT)/tests/tests/hello/crt0.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/s123.s: mdrsave-yos.c $(XCC) | $(BUILD_DIR)
	$(XCC) $(XCCFLAGS) -DTEST_NAME='"t123"' -DTEST_LEN=123 -DTEST_SECTORS=1 -o $@ $<

$(BUILD_DIR)/s512.s: mdrsave-yos.c $(XCC) | $(BUILD_DIR)
	$(XCC) $(XCCFLAGS) -DTEST_NAME='"t512"' -DTEST_LEN=512 -DTEST_SECTORS=1 -o $@ $<

$(BUILD_DIR)/s777.s: mdrsave-yos.c $(XCC) | $(BUILD_DIR)
	$(XCC) $(XCCFLAGS) -DTEST_NAME='"t777"' -DTEST_LEN=777 -DTEST_SECTORS=2 -o $@ $<

$(BUILD_DIR)/%.rel: $(BUILD_DIR)/%.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@
