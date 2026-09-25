REPO_ROOT ?= $(realpath ../../..)
BUILD_DIR ?= $(REPO_ROOT)/build/yos-shell-c-sample
DIST_DIR ?= $(REPO_ROOT)/bin/y/arch/48
XTOOLS ?= $(REPO_ROOT)/bin/x/bin

XCC := $(XTOOLS)/xcc
XPROG := $(XTOOLS)/xprog
XL := $(BUILD_DIR)/shell.xl
OUTPUT := $(DIST_DIR)/shell.sys

.PHONY: all clean
all: $(OUTPUT)

$(XL): shell.c
	mkdir -p $(dir $@)
	$(XCC) --mode=sdcc --platform=yos -Os -std=c11 $< -o $@

$(OUTPUT): $(XL)
	mkdir -p $(dir $@)
	$(XPROG) --process --name shell --stack-size 512 --min-os 6 $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OUTPUT)
