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

all: $(BUILD_DIR)/hello.com $(BUILD_DIR)/trygetc.com $(BUILD_DIR)/argv.com \
	$(BUILD_DIR)/argv-os.com $(BUILD_DIR)/argv-of.com \
	$(BUILD_DIR)/argv-stk.com

$(BUILD_DIR)/hello.com: hello.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) hello.c --platform=cpm3 --oformat=binary -o $@

$(BUILD_DIR)/trygetc.com: trygetchar.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) trygetchar.c --platform=cpm3 --oformat=binary -o $@

$(BUILD_DIR)/argv.com: argv.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) argv.c --platform=cpm3 --oformat=binary -o $@

$(BUILD_DIR)/argv-os.com: argv.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) argv.c --platform=cpm3 --oformat=binary -Os -o $@

$(BUILD_DIR)/argv-of.com: argv.c
	@mkdir -p $(BUILD_DIR)
	$(XCC) argv.c --platform=cpm3 --oformat=binary -Of -o $@

$(BUILD_DIR)/argv-stk.com: argv.c argv_stack.s
	@mkdir -p $(BUILD_DIR)
	$(XCC) argv.c argv_stack.s -DARGV_STACK_ABI=1 --sdcccall 0 \
		--platform=cpm3 --oformat=binary -o $@

test: all
	@if ! command -v $(TNYLPO) >/dev/null 2>&1; then \
	  echo "SKIP: hello-cpm (tnylpo not found — install from https://gitlab.com/gbrein/tnylpo)"; \
	  exit 0; \
	fi; \
	out="$$(cd $(BUILD_DIR) && $(TNYLPO) hello)"; \
	if [ "$$out" = "Hello, CP/M world!" ]; then \
	  echo "PASS: hello-cpm"; \
	else \
	  echo "FAIL: hello-cpm (got: $$out)"; exit 1; \
	fi; \
	out="$$(tail -f /dev/null | (cd $(BUILD_DIR) && $(TNYLPO) trygetc))"; \
	if [ "$$out" = "IDLE" ]; then \
	  echo "PASS: hello-cpm trygetchar idle"; \
	else \
	  echo "FAIL: hello-cpm trygetchar idle (got: $$out)"; exit 1; \
	fi; \
	out="$$(printf x | (cd $(BUILD_DIR) && $(TNYLPO) trygetc))"; \
	if [ "$$out" = "xREADY" ]; then \
	  echo "PASS: hello-cpm trygetchar ready"; \
	else \
	  echo "FAIL: hello-cpm trygetchar ready (got: $$out)"; exit 1; \
	fi; \
	for prog in argv argv-os argv-of argv-stk; do for args in \
	  "" \
	  "BASIC ALPHA BETA" \
	  "SPACES   ALPHA   BETA" \
	  'QUOTED "TWO WORDS"' \
	  'EMPTY ""' \
	  "DURABLE AFTER" \
	  "BOUNDARY XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
	  "MANY A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A A"; do \
	  out="$$(cd $(BUILD_DIR) && $(TNYLPO) "$$prog" "$$args")"; \
	  if [ "$$out" != "PASS" ]; then \
	    echo "FAIL: hello-cpm $$prog ($$args; got: $$out)"; exit 1; \
	  fi; \
	done; done; \
	echo "PASS: hello-cpm argv"

clean:
	rm -rf $(BUILD_DIR)
