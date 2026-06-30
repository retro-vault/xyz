PYTHON ?= python3

_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
X_ROOT ?= $(abspath $(_THIS_DIR)/../../..)
REPO_ROOT ?= $(abspath $(X_ROOT)/..)
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin

XEMUTEST_DIR := $(X_ROOT)/tests/tools/xemutest
XEMUTEST_BIN := $(BUILD_DIR)/tests/tools/xemutest/xemutest
SUITE_ROOT := $(X_ROOT)/tests/tests/z88dk
XCC_BIN ?= $(HOST_BIN_DIR)/xcc
UPSTREAM_GIT_URL ?= https://github.com/z88dk/z88dk
UPSTREAM_GIT_REF ?= master
UPSTREAM_CLONE_DIR ?= $(BUILD_DIR)/z88dk-upstream

.PHONY: all generate test clean stage-toolchain sync-upstream sync-upstream-git

all: generate

stage-toolchain:
	$(MAKE) -C $(XEMUTEST_DIR) stage-toolchain all

generate: stage-toolchain
	XCC=$(XCC_BIN) $(PYTHON) $(SUITE_ROOT)/scripts/generate_z88dk_cases.py

test: generate
	$(XEMUTEST_BIN) --xcc $(XCC_BIN) --suite $(SUITE_ROOT)

clean:
	rm -rf $(SUITE_ROOT)/cases $(SUITE_ROOT)/generated $(SUITE_ROOT)/build
	rm -f $(SUITE_ROOT)/docs/status.md

sync-upstream:
	@if [ -z "$(UPSTREAM)" ]; then \
		echo "usage: make sync-upstream UPSTREAM=/path/to/z88dk"; \
		exit 1; \
	fi
	rsync -a --delete "$(UPSTREAM)/test/" "$(SUITE_ROOT)/upstream/test/"
	rsync -a --delete "$(UPSTREAM)/testsuite/" "$(SUITE_ROOT)/upstream/testsuite/"
	rm -rf "$(SUITE_ROOT)/upstream/test/suites/zx"

sync-upstream-git:
	@if [ ! -d "$(UPSTREAM_CLONE_DIR)/.git" ]; then \
		git clone --filter=blob:none --branch "$(UPSTREAM_GIT_REF)" "$(UPSTREAM_GIT_URL)" "$(UPSTREAM_CLONE_DIR)"; \
	else \
		git -C "$(UPSTREAM_CLONE_DIR)" fetch origin "$(UPSTREAM_GIT_REF)"; \
		git -C "$(UPSTREAM_CLONE_DIR)" checkout -f "origin/$(UPSTREAM_GIT_REF)"; \
	fi
	$(MAKE) sync-upstream UPSTREAM="$(UPSTREAM_CLONE_DIR)"
