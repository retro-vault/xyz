# ---------------------------------------------------------------------------
# Makefile for hello-cpm — builds HELLO.COM, a CP/M 3 transient program.
#
# Build:  make            (requires staged xtools in bin/x)
# Test:   make test       (requires tnylpo in PATH or TNYLPO=<path>)
#         tnylpo: https://gitlab.com/gbrein/tnylpo
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
DIST_DIR  ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
BUILD_DIR := $(ROOT)/build/hello-cpm
XCC       := $(HOST_BIN_DIR)/xcc
TNYLPO    ?= tnylpo

.PHONY: all test clean

all: $(BUILD_DIR)/hello.com

$(BUILD_DIR)/hello.com: hello.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) hello.c -T $(DIST_DIR)/z80/lib/linker.lk --oformat=binary -o $@

test: $(BUILD_DIR)/hello.com
	@command -v $(TNYLPO) >/dev/null 2>&1 || { \
	  echo "hello-cpm: tnylpo not found — install from https://gitlab.com/gbrein/tnylpo"; \
	  exit 1; }
	@out="$$(cd $(BUILD_DIR) && $(TNYLPO) hello)"; \
	if [ "$$out" = "Hello, CP/M world!" ]; then \
	  echo "PASS: hello-cpm"; \
	else \
	  echo "FAIL: hello-cpm (got: $$out)"; exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
