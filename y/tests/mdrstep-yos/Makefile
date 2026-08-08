ROOT      ?= $(realpath ../..)
Y_ROOT    ?= $(ROOT)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(realpath ../../..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR := $(REPO_ROOT)/build/mdrstep-yos
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
X_DIST_DIR ?= $(REPO_ROOT)/bin/x
X_HOST_BIN_DIR ?= $(X_DIST_DIR)/bin
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/mdrstep-yos
XAS     := $(X_HOST_BIN_DIR)/xas
XLD     := $(X_HOST_BIN_DIR)/xld
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
IMAGE     := $(ZX_MDR_DIR)/mdrstep.mdr

.DEFAULT_GOAL := all

APPS := s123 s512 s777 l123 l512 l777

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(IMAGE)

.PHONY: all
all: $(addprefix $(BIN_DIR)/,$(addsuffix .app,$(APPS)))

XASFLAGS := --mode=sdcc -g
XLDFLAGS := --mode=sdcc -v -m -nostdlib --no-default-runtime -e _entry

$(XAS) $(XLD):
	$(MAKE) -C $(REPO_ROOT) x

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/%.app: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/%.rel $(XLD) | $(BIN_DIR)
	$(XLD) $(XLDFLAGS) \
	    -o $@ \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/$*.rel

.PHONY: image
image: all $(MDR_TOOL)
	@mkdir -p $(ZX_MDR_DIR)
	$(MDR_TOOL) create $(IMAGE) MDRSTEP
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s123.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s512.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s777.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/l123.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/l512.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/l777.app
	$(MDR_TOOL) dir $(IMAGE)

$(MDR_TOOL):
	$(MAKE) -C $(Y_ROOT)/pkg/microdrive all \
		ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) BUILD_DIR=$(REPO_ROOT)/build \
		DIST_DIR=$(Y_DIST_DIR) HOST_BIN_DIR=$(Y_HOST_BIN_DIR) \
		PUBLIC_LIB_DIR=$(Y_DIST_DIR)/lib PUBLIC_INCLUDE_DIR=$(Y_DIST_DIR)/include

$(BUILD_DIR)/crt0.rel: crt0tiny.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@

$(BUILD_DIR)/s123.s: save.s.in
	@mkdir -p $(BUILD_DIR)
	sed -e 's/@MODULE@/s123/g' -e 's/@NAME@/t123/g' -e 's/@LEN@/123/g' $< > $@

$(BUILD_DIR)/s512.s: save.s.in
	@mkdir -p $(BUILD_DIR)
	sed -e 's/@MODULE@/s512/g' -e 's/@NAME@/t512/g' -e 's/@LEN@/512/g' $< > $@

$(BUILD_DIR)/s777.s: save.s.in
	@mkdir -p $(BUILD_DIR)
	sed -e 's/@MODULE@/s777/g' -e 's/@NAME@/t777/g' -e 's/@LEN@/777/g' $< > $@

$(BUILD_DIR)/l123.s: load.s.in
	@mkdir -p $(BUILD_DIR)
	sed -e 's/@MODULE@/l123/g' -e 's/@NAME@/t123/g' -e 's/@LEN@/123/g' $< > $@

$(BUILD_DIR)/l512.s: load.s.in
	@mkdir -p $(BUILD_DIR)
	sed -e 's/@MODULE@/l512/g' -e 's/@NAME@/t512/g' -e 's/@LEN@/512/g' $< > $@

$(BUILD_DIR)/l777.s: load.s.in
	@mkdir -p $(BUILD_DIR)
	sed -e 's/@MODULE@/l777/g' -e 's/@NAME@/t777/g' -e 's/@LEN@/777/g' $< > $@

$(BUILD_DIR)/%.rel: $(BUILD_DIR)/%.s $(XAS) | $(BUILD_DIR)
	$(XAS) $(XASFLAGS) $< -o $@
