# ---------------------------------------------------------------------------
# Makefile for hello-cpm — builds HELLO.COM, a CP/M 3 transient program.
#
# Build:  make            (requires staged x tools in bin/x)
# Test:   make test       (uses tnylpo in PATH or TNYLPO=<path>, skips if absent)
#         tnylpo: https://gitlab.com/gbrein/tnylpo
# ---------------------------------------------------------------------------

_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
X_ROOT    ?= $(abspath $(_THIS_DIR)/../../..)
REPO_ROOT ?= $(abspath $(X_ROOT)/..)
DIST_DIR  ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
BUILD_DIR := $(REPO_ROOT)/build/hello-cpm
XCC       := $(HOST_BIN_DIR)/xcc
TNYLPO    ?= tnylpo

.PHONY: all test clean

all: $(BUILD_DIR)/hello.com

$(BUILD_DIR)/hello.com: hello.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) hello.c -T $(DIST_DIR)/z80/lib/linker.lk --oformat=binary -o $@

test: $(BUILD_DIR)/hello.com
	@if ! command -v $(TNYLPO) >/dev/null 2>&1; then \
	  echo "SKIP: hello-cpm (tnylpo not found — install from https://gitlab.com/gbrein/tnylpo)"; \
	  exit 0; \
	fi; \
	out="$$(cd $(BUILD_DIR) && $(TNYLPO) hello)"; \
	if [ "$$out" = "Hello, CP/M world!" ]; then \
	  echo "PASS: hello-cpm"; \
	else \
	  echo "FAIL: hello-cpm (got: $$out)"; exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
