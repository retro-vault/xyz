# ---------------------------------------------------------------------------
# Root Makefile — delegates only, compiles nothing here.
# Add new sub-projects to SUBDIRS when they become ready to build.
#
# Usage:
#   make        — build everything
#   make clean  — remove all build artifacts
# ---------------------------------------------------------------------------

# Linux only.
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Prerequisite checks.
$(if $(shell which docker),,$(error "docker not found. Please install Docker."))
$(if $(shell which gcc),,$(error "gcc not found. Please install gcc."))

# Repository root and shared output directories (exported for sub-makes).
export ROOT      := $(realpath .)
export BUILD_DIR := $(ROOT)/build
export BIN_DIR   := $(ROOT)/bin

# Compilable sub-projects — order matters (dependencies first).
# Add new entries here as sub-projects become ready; do not add make targets.
SUBDIRS := tools src/yos

.PHONY: all
all:
	@for d in $(SUBDIRS); do \
	    echo "==> $$d"; \
	    $(MAKE) -C $$d || exit 1; \
	done

.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)
