ROOT      ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
BUILD_DIR := $(ROOT)/build/mdrsave-yos
DIST_DIR  ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
ZX_APPS_DIR ?= $(DIST_DIR)/targets/zxspectrum/apps
ZX_MDR_DIR ?= $(DIST_DIR)/targets/zxspectrum/mdr
BIN_DIR   := $(ZX_APPS_DIR)/mdrsave-yos
XLINK     := $(HOST_BIN_DIR)/xlink
MDR_TOOL  := $(HOST_BIN_DIR)/microdrive
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
	    -v $(ROOT):$(ROOT) \
	    -w $(THIS_DIR) \
	    $(DOCKER_IMAGE) \
	    make IN_DOCKER=1 ROOT=$(ROOT) APP=$*
	$(XLINK) -v -m -e _entry \
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
           -I$(ROOT)/include \
           -I$(ROOT)/src/yos \
           -I$(ROOT)/src/yos/include
ASFLAGS := -xlos -g

.PHONY: all
all: $(BUILD_DIR)/crt0.rel $(BUILD_DIR)/$(APP).rel

$(BUILD_DIR)/crt0.rel: ../hello/crt0.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/s123.rel: mdrsave-yos.c
	$(CC) -c $(CFLAGS) -DTEST_NAME='"t123"' -DTEST_LEN=123 -DTEST_SECTORS=1 -o $@ $<

$(BUILD_DIR)/s512.rel: mdrsave-yos.c
	$(CC) -c $(CFLAGS) -DTEST_NAME='"t512"' -DTEST_LEN=512 -DTEST_SECTORS=1 -o $@ $<

$(BUILD_DIR)/s777.rel: mdrsave-yos.c
	$(CC) -c $(CFLAGS) -DTEST_NAME='"t777"' -DTEST_LEN=777 -DTEST_SECTORS=2 -o $@ $<

endif
