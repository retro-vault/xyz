# We only allow compilation on linux!
ifneq ($(shell uname), Linux)
$(error OS must be Linux!)
endif

# Check if all required tools are on the system.
REQUIRED = sdcc sdar sdasz80 gcc fuse truncate
K := $(foreach exec,$(REQUIRED),\
    $(if $(shell which $(exec)),,$(error "$(exec) not found. Please install or add to path.")))

# Configure output folders for linux, don't work on other systems.
export BUILD_DIR	= $(shell pwd)/build
export BIN_DIR		= $(shell pwd)/bin
export MKDIR		= mkdir -p
export RMDIR		= rm -r -f

# Build directories (all)
BUILDDIRS = $(BUILD_DIR) $(BIN_DIR) $(BIN_DIR)/tools

# Targets are subdirs.
SUBDIRS = tools libs src

# Rules.
.PHONY: all clean $(BUILDDIRS) $(SUBDIRS)

all: $(BUILDDIRS) $(SUBDIRS)

clean:
	$(RMDIR) $(BUILD_DIR)
	$(RMDIR) $(BIN_DIR)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(BUILDDIRS):
	$(MKDIR) $@ 