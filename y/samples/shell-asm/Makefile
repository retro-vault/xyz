REPO_ROOT ?= $(realpath ../../..)
BUILD_DIR ?= $(REPO_ROOT)/build/yos-shell-asm-sample
DIST_DIR ?= $(REPO_ROOT)/bin/y/arch/48
XTOOLS ?= $(REPO_ROOT)/bin/x/bin

XAS := $(XTOOLS)/xas
XLD := $(XTOOLS)/xld
XPROG := $(XTOOLS)/xprog
OBJECT := $(BUILD_DIR)/shell.rel
XL := $(BUILD_DIR)/shell.xl
OUTPUT := $(DIST_DIR)/shell-asm.sys

.PHONY: all clean
all: $(OUTPUT)

$(OBJECT): shell.s
	mkdir -p $(dir $@)
	$(XAS) --mode=sdcc -I$(REPO_ROOT)/y/include $< -o $@

$(XL): $(OBJECT) linker.lk
	$(XLD) --mode=sdcc -f xl -nostdlib --no-default-runtime \
		-T linker.lk -Map=$(BUILD_DIR)/shell.map -o $@ $<

$(OUTPUT): $(XL)
	mkdir -p $(dir $@)
	$(XPROG) --process --name shell --stack-size 512 --min-os 1 $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OUTPUT)
