ROOT      ?= $(realpath ../..)
Y_ROOT    ?= $(ROOT)
THIS_DIR  := $(realpath .)
REPO_ROOT ?= $(realpath ../../..)
X_ROOT    ?= $(REPO_ROOT)/x
BUILD_DIR := $(REPO_ROOT)/build/mdrsave-yos
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
Y_DIST_DIR ?= $(REPO_ROOT)/bin/y
Y_HOST_BIN_DIR ?= $(Y_DIST_DIR)/bin
ZX_APPS_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/apps
ZX_MDR_DIR ?= $(REPO_ROOT)/bin/z/z80/spectrum/bin/mdr
BIN_DIR   := $(ZX_APPS_DIR)/mdrsave-yos
XLD     := $(HOST_BIN_DIR)/xld
MDR_TOOL  := $(Y_HOST_BIN_DIR)/microdrive
IMAGE     := $(ZX_MDR_DIR)/mdrsave.mdr

DOCKER_IMAGE := wischner/sdcc-z80-zx-spectrum:latest

.DEFAULT_GOAL := all

APPS := s123 s512 s777

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(IMAGE)

ifndef IN_DOCKER
.PHONY: all
all: $(addprefix $(BIN_DIR)/,$(addsuffix .app,$(APPS)))

$(BIN_DIR)/%.app:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	docker run --rm \
	    -u $(shell id -u):$(shell id -g) \
	    -v $(REPO_ROOT):$(REPO_ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 ROOT=$(ROOT) REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) X_ROOT=$(X_ROOT) APP=$*
	$(XLD) -v -m -nostdlib -e _entry \
	    -o $@ \
	    $(BUILD_DIR)/crt0.rel \
	    $(BUILD_DIR)/$*.rel

.PHONY: image
image: all
	@mkdir -p $(ZX_MDR_DIR)
	$(MDR_TOOL) create $(IMAGE) MDRSAVE
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s123.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s512.app
	$(MDR_TOOL) put $(IMAGE) $(BIN_DIR)/s777.app
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
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/$(APP).rel

$(BUILD_DIR)/crt0.rel: $(X_ROOT)/tests/hello/crt0.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/s123.rel: mdrsave-yos.c
	$(CC) -c $(CFLAGS) -DTEST_NAME='"t123"' -DTEST_LEN=123 -DTEST_SECTORS=1 -o $@ $<

$(BUILD_DIR)/s512.rel: mdrsave-yos.c
	$(CC) -c $(CFLAGS) -DTEST_NAME='"t512"' -DTEST_LEN=512 -DTEST_SECTORS=1 -o $@ $<

$(BUILD_DIR)/s777.rel: mdrsave-yos.c
	$(CC) -c $(CFLAGS) -DTEST_NAME='"t777"' -DTEST_LEN=777 -DTEST_SECTORS=2 -o $@ $<

endif
